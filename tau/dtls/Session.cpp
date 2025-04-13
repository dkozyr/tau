#include "tau/dtls/Session.h"
#include "tau/common/String.h"
#include "tau/common/Exception.h"
#include "tau/common/Log.h"
#include <openssl/err.h>

// #include <openssl/x509_vfy.h>

namespace tau::dtls {

Session::Session(Dependencies&& deps, Options&& options)
    : _deps(deps)
    , _options(std::move(options)) {

    _ctx = SSL_CTX_new(DTLS_method());
    if(!_ctx) {
        TAU_EXCEPTION(std::runtime_error, "SSL_CTX_new failed, error: " << ERR_error_string(ERR_get_error(), NULL));
    }
    SSL_CTX_set_min_proto_version(_ctx, DTLS1_2_VERSION);
    SSL_CTX_set_max_proto_version(_ctx, DTLS1_2_VERSION);

    if(auto error = SSL_CTX_use_certificate(_ctx, _cert.GetCertificate()) <= 0) {
        TAU_EXCEPTION(std::runtime_error, "SSL_CTX_use_certificate failed, error: " << error
            << ", message: " << ERR_error_string(ERR_get_error(), NULL));
    }
    if(auto error = SSL_CTX_use_PrivateKey(_ctx, _cert.GetPrivateKey()) <= 0) {
        TAU_EXCEPTION(std::runtime_error, "SSL_CTX_use_PrivateKey failed, error: " << error
            << ", message: " << ERR_error_string(ERR_get_error(), NULL));
    }

    if(!SSL_CTX_set_cipher_list(_ctx, "DEFAULT:!NULL:!aNULL:!SHA256:!SHA384:!aECDH:!AESGCM+AES256:!aPSK:!3DES")) {
        TAU_EXCEPTION(std::runtime_error, "SSL_CTX_set_cipher_list failed, error: " << ERR_error_string(ERR_get_error(), NULL));
    }

    //TODO: do we need it?
    // Verify private key matches certificate (optional but recommended)
    if(auto error = SSL_CTX_check_private_key(_ctx) <= 0) {
        TAU_EXCEPTION(std::runtime_error, "SSL_CTX_check_private_key failed, error: " << error
            << ", message: " << ERR_error_string(ERR_get_error(), NULL));
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

    const auto digest = _cert.GetDigestSha256();
    TAU_LOG_INFO(_options.log_ctx << "fingerprint: " << ToHexDump(digest.data(), digest.size(), ':'));
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

}
