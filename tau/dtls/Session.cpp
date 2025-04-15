#include "tau/dtls/Session.h"
#include "tau/common/String.h"
#include "tau/common/Exception.h"
#include "tau/common/Log.h"
#include <openssl/err.h>
#include <unordered_map>

namespace tau::dtls {

const std::unordered_map<Session::SrtpProfile, size_t> kProfileKeyingMaterialSize = {
    {Session::SrtpProfile::kAes128CmSha1_80, 20 + 10},
    {Session::SrtpProfile::kAes128CmSha1_32, 20 + 10},
    // {Session::SrtpProfile::kAes256CmSha1_80, 20 + 10},
    // {Session::SrtpProfile::kAes256CmSha1_32, 20 + 10},
    // {Session::SrtpProfile::kAeadAes128Gcm,   16 + 12},
    // {Session::SrtpProfile::kAeadAes256Gcm,   32 + 12},
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

    _bio_read = BIO_new(BIO_s_mem());
    _bio_write = BIO_new(BIO_s_mem());
    SSL_set_bio(_ssl, _bio_read, _bio_write);

    SSL_set_app_data(_ssl, this);

    // DTLSv1_set_initial_timeout_duration(_ssl, _handshake_timeout_ms); //TODO: process timeouts
}

Session::~Session() {
    TAU_LOG_DEBUG(_options.log_ctx << "SSL_state: " << SSL_state_string(_ssl));
    SSL_free(_ssl);
    SSL_CTX_free(_ctx);
}

void Session::Process() {
    if(_state == State::kFailed) { return; }
    if((_options.type == Type::kClient) && (_state == State::kWaiting)) {
        _state = State::kConnecting;
        _state_callback(_state);
    }

    const auto code = (_options.type == Type::kServer) ? SSL_accept(_ssl) : SSL_connect(_ssl);
    if(code <= 0) {
        const auto error = SSL_get_error(_ssl, code);
        TAU_LOG_DEBUG(_options.log_ctx << "code: " << code << ", error: " << error);
        switch(error) {
            case SSL_ERROR_SSL:
            case SSL_ERROR_SYSCALL:
                _state = State::kFailed;
                _state_callback(_state);
                break;
            default:
                break;
        }
    } else {
        if(_state == State::kConnecting) {
            _state = State::kConnected;
            _state_callback(_state);
        }

        while((SSL_pending(_ssl) > 0) || (BIO_ctrl_pending(_bio_read))) {
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
    Process();
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

    Process();
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
    const auto keying_material_size = kProfileKeyingMaterialSize.at(*profile);
    std::vector<uint8_t> keying_material(2 * keying_material_size);
    if(!SSL_export_keying_material(_ssl, keying_material.data(), keying_material.size(), kLabel.data(), kLabel.size(), NULL, 0, 0)) {
        return {};
    }

    if((encryption && (_options.type == Type::kClient)) || (!encryption && (_options.type == Type::kServer))) {
        keying_material.resize(keying_material_size);
    } else {
        keying_material.erase(keying_material.begin() + keying_material_size);
    }
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
