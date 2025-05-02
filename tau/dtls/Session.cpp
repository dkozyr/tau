#include "tau/dtls/Session.h"
#include "tau/common/String.h"
#include "tau/common/Exception.h"
#include "tau/common/Log.h"
#include <openssl/err.h>
#include <unordered_map>

namespace tau::dtls {

struct KeyingMaterialSize {
    size_t key;
    size_t salt;
};
const std::unordered_map<Session::SrtpProfile, KeyingMaterialSize> kProfileKeyingMaterialSize = {
    {Session::SrtpProfile::kAes128CmSha1_80, KeyingMaterialSize{.key = 16, .salt = 14}},
    {Session::SrtpProfile::kAes128CmSha1_32, KeyingMaterialSize{.key = 16, .salt = 14}},
    // {Session::SrtpProfile::kAes256CmSha1_80, KeyingMaterialSize{.key = 16, .salt = 14}},
    // {Session::SrtpProfile::kAes256CmSha1_32, KeyingMaterialSize{.key = 16, .salt = 14}},
    // {Session::SrtpProfile::kAeadAes128Gcm,   KeyingMaterialSize{.key = 16, .salt = 12}},
    // {Session::SrtpProfile::kAeadAes256Gcm,   KeyingMaterialSize{.key = 32, .salt = 12}},
};

Session::Session(Dependencies&& deps, Options&& options)
    : _deps(deps)
    , _options(std::move(options)) {

    _ctx = SSL_CTX_new(DTLS_method());
    if(!_ctx) {
        TAU_EXCEPTION(std::runtime_error, "SSL_CTX_new failed, error: " << ERR_error_string(ERR_get_error(), NULL));
    }
    SSL_CTX_set_min_proto_version(_ctx, DTLS1_2_VERSION);
    SSL_CTX_set_max_proto_version(_ctx, DTLS1_2_VERSION);

    if(auto error = SSL_CTX_use_certificate(_ctx, _deps.certificate.GetCertificate()) <= 0) {
        TAU_EXCEPTION(std::runtime_error, "SSL_CTX_use_certificate failed, error: " << error
            << ", message: " << ERR_error_string(ERR_get_error(), NULL));
    }
    if(auto error = SSL_CTX_use_PrivateKey(_ctx, _deps.certificate.GetPrivateKey()) <= 0) {
        TAU_EXCEPTION(std::runtime_error, "SSL_CTX_use_PrivateKey failed, error: " << error
            << ", message: " << ERR_error_string(ERR_get_error(), NULL));
    }

    if(!_options.srtp_profiles.empty()) {
        if(auto error = SSL_CTX_set_tlsext_use_srtp(_ctx, _options.srtp_profiles.c_str())) {
            TAU_EXCEPTION(std::runtime_error, "SSL_CTX_set_tlsext_use_srtp failed, error: " << error
                << ", message: " << ERR_error_string(ERR_get_error(), NULL));
        }
    }

    if(!_options.remote_peer_cert_digest.empty()) {
        SSL_CTX_set_verify(_ctx, SSL_VERIFY_PEER, OnVerifyPeerStatic);
    }

    _ssl = SSL_new(_ctx);
    if(!_ssl) {
        TAU_EXCEPTION(std::runtime_error, "SSL_new failed, error: " << ERR_error_string(ERR_get_error(), NULL));
    }

    constexpr auto kDtlsMtuLimit = 1100;
    SSL_set_mtu(_ssl, kDtlsMtuLimit);
    DTLS_set_link_mtu(_ssl, kDtlsMtuLimit);

    _bio_read = BIO_new(BIO_s_mem());
    _bio_write = BIO_new(BIO_s_mem());
    SSL_set_bio(_ssl, _bio_read, _bio_write);

    SSL_set_app_data(_ssl, this);
}

Session::~Session() {
    TAU_LOG_DEBUG(_options.log_ctx << "SSL_state: " << SSL_state_string(_ssl));
    _state = State::kFailed;
    SSL_free(_ssl);
    SSL_CTX_free(_ctx);
}

void Session::Process() {
    switch(_state) {
        case State::kWaiting:
            if(_options.type == Type::kClient) {
                _state = State::kConnecting;
                _state_callback(_state);
            }
            break;
        case State::kConnecting:
            break;
        case State::kConnected: return;
        case State::kFailed: return;
    }

    const auto code = (_options.type == Type::kServer) ? SSL_accept(_ssl) : SSL_connect(_ssl);
    if(code <= 0) {
        const auto error = SSL_get_error(_ssl, code);
        TAU_LOG_DEBUG(_options.log_ctx << "code: " << code << ", error: " << error << ", state: " << SSL_state_string(_ssl));
        switch(error) {
            case SSL_ERROR_WANT_READ:
                while((SSL_pending(_ssl) > 0) || (BIO_ctrl_pending(_bio_read))) {
                    TAU_LOG_INFO("SSL_pending(_ssl): " << SSL_pending(_ssl) << ", BIO_ctrl_pending(_bio_read): " << BIO_ctrl_pending(_bio_read));
                    auto packet = Buffer::Create(_deps.udp_allocator);
                    auto view = packet.GetViewWithCapacity();
                    auto recv_read = SSL_read(_ssl, view.ptr, view.size);
                    if(recv_read > 0) {
                        packet.SetSize(recv_read);
                        _recv_callback(std::move(packet));
                    } else {
                        break;
                    }
                }
                break;
            case SSL_ERROR_WANT_WRITE:
                if(auto pending_size = BIO_ctrl_pending(_bio_write)) {
                    auto packet = Buffer::Create(_deps.udp_allocator);
                    auto view = packet.GetViewWithCapacity();
                    auto size = BIO_read(_bio_write, view.ptr, pending_size);
                    if(size > 0) {
                        packet.SetSize(size);
                        _send_callback(std::move(packet));
                    } else {
                        TAU_LOG_WARNING(_options.log_ctx << "BIO_read size: " << size);
                    }
                }
                break;
            case SSL_ERROR_SSL:
                _state = State::kFailed;
                _state_callback(_state);
                break;
            case SSL_ERROR_SYSCALL:
                return;
            default:
                break;
        }
    } else {
        if(_state == State::kConnecting) {
            _state = State::kConnected;
            _state_callback(_state);
        }
    }

    if(auto pending_size = BIO_ctrl_pending(_bio_write)) {
        auto packet = Buffer::Create(_deps.udp_allocator);
        auto view = packet.GetViewWithCapacity();
        auto size = BIO_read(_bio_write, view.ptr, pending_size);
        if(size > 0) {
            packet.SetSize(size);
            _send_callback(std::move(packet));
        } else {
            TAU_LOG_WARNING(_options.log_ctx << "BIO_read size: " << size);
        }
    }
}

void Session::Stop() {
    auto error = SSL_shutdown(_ssl);
    if(error < 0) {
        TAU_LOG_INFO(_options.log_ctx << "error: " << error << ", message: " << ERR_error_string(ERR_get_error(), NULL));
    }
}

bool Session::Send(Buffer&& packet) {
    return Send(ToConst(packet.GetView()));
}

bool Session::Send(const BufferViewConst& packet_view) {
    auto size = SSL_write(_ssl, packet_view.ptr, packet_view.size);
    if(size <= 0) {
        TAU_LOG_WARNING(_options.log_ctx << "size: " << size << ", error: " << ERR_error_string(ERR_get_error(), NULL));
        return false;
    }
    return true;
}

void Session::Recv(Buffer&& packet) {
    if((_options.type == Type::kServer) && (_state == State::kWaiting)) {
        _state = State::kConnecting;
        _state_callback(_state);
    }

    auto view = packet.GetView();
    auto size = BIO_write(_bio_read, view.ptr, view.size);
    if(size <= 0) {
        TAU_LOG_WARNING(_options.log_ctx << "BIO_write size: " << size);
    }
}

std::optional<Timepoint> Session::GetTimeout() {
    if(_state == State::kConnecting) {
        timeval timeout;
        if(DTLSv1_get_timeout(_ssl, &timeout) == 1) {
            auto tp = timeout.tv_sec * kSec + timeout.tv_usec * kMicro;
            if(tp < kFailureTimeout) {
                return tp;
            } else {
                _state = State::kFailed;
                _state_callback(_state);
            }
        }
    }
    return std::nullopt;
}

std::optional<Session::SrtpProfile> Session::GetSrtpProfile() const {
    if(_state != State::kConnected) {
        return std::nullopt;
    }
    auto selected_profile = SSL_get_selected_srtp_profile(_ssl);
    if(!selected_profile) {
        TAU_LOG_WARNING("SSL_get_selected_srtp_profile failed, error: " << ERR_error_string(ERR_get_error(), NULL));
        return std::nullopt;
    }

    const auto profile = static_cast<SrtpProfile>(selected_profile->id);
    switch(profile) {
        case SrtpProfile::kAes128CmSha1_80:
        case SrtpProfile::kAes128CmSha1_32:
            return profile;
    }
    return std::nullopt;
}

std::vector<uint8_t> Session::GetKeyingMaterial(bool encryption) const {
    auto profile = GetSrtpProfile();
    if(!profile) {
        return {};
    }

    constexpr std::string_view kLabel = "EXTRACTOR-dtls_srtp";
    const auto& [key_size, salt_size] = kProfileKeyingMaterialSize.at(*profile);
    std::vector<uint8_t> keying_material(2 * (key_size + salt_size));
    if(!SSL_export_keying_material(_ssl, keying_material.data(), keying_material.size(), kLabel.data(), kLabel.size(), NULL, 0, 0)) {
        return {};
    }

    // https://www.rfc-editor.org/rfc/rfc5764.html#section-4.2
    if((encryption && (_options.type == Type::kClient)) || (!encryption && (_options.type == Type::kServer))) {
        std::memcpy(keying_material.data() + key_size, keying_material.data() + 2 * key_size, salt_size);
    } else {
        std::memcpy(keying_material.data(), keying_material.data() + key_size, key_size);
        std::memcpy(keying_material.data() + key_size, keying_material.data() + 2 * key_size + salt_size, salt_size);
    }
    keying_material.resize(key_size + salt_size);
    return keying_material;
}

int Session::OnVerifyPeerStatic(int preverify_ok, X509_STORE_CTX* x509_ctx) {
    if(auto ssl = (SSL*)X509_STORE_CTX_get_ex_data(x509_ctx, SSL_get_ex_data_X509_STORE_CTX_idx())) {
        if(auto self = SSL_get_app_data(ssl)) {
            return static_cast<Session*>(self)->OnVerifyPeer(preverify_ok, x509_ctx);
        }
    }
    return 0;
}

int Session::OnVerifyPeer(int, X509_STORE_CTX* x509_ctx) {
    if(auto cert = X509_STORE_CTX_get_current_cert(x509_ctx)) {
        std::vector<uint8_t> digest(EVP_MAX_MD_SIZE);
        uint32_t size = 0;
        if(X509_digest(cert, EVP_sha256(), digest.data(), &size)) {
            return (_options.remote_peer_cert_digest == ToHexDump(digest.data(), size, ':'));
        }
    }
    return 0;
}

}
