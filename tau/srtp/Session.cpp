#include "tau/srtp/Session.h"
#include "tau/common/Exception.h"
#include "tau/common/Log.h"
#include <cstring>

namespace tau::srtp {

Session::Session(Options&& options)
    : _log_ctx(options.log_ctx) {
    srtp_policy_t policy;
    if(auto error = srtp_policy_create(&policy)) {
        TAU_LOG_ERROR(_log_ctx << "srtp_policy_create failed, error: " << error);
        return;
    }

    srtp_policy_set_profile(policy, options.profile);
    srtp_policy_set_allow_repeat_tx(policy, true);
    srtp_policy_set_window_size(policy, kRtxWindowSize);
    srtp_policy_set_ssrc(policy, srtp_ssrc_t{
        (options.type == Type::kEncryptor)
            ? srtp_ssrc_type_t::ssrc_any_outbound
            : srtp_ssrc_type_t::ssrc_any_inbound,
        0
    });

    srtp_policy_add_key(policy,
        options.key.data(), options.key.size(),
        options.salt.data(), options.salt.size(),
        nullptr, 0);

    if(auto error = srtp_create(&_session, policy)) {
        // on srtp_err_status_init_fail try to init srtp first
        TAU_LOG_ERROR(_log_ctx << "srtp_create failed, error: " << error);
    }

    srtp_policy_destroy(policy);
}

Session::~Session() {
    if(_session) {
        srtp_dealloc(_session);
    }
}

bool Session::IsValid() const {
    return (_session != nullptr);
}

bool Session::Encrypt(Buffer&& packet, bool is_rtp) {
    auto view = packet.GetView();
    size_t encrypted_size = packet.GetCapacity();
    if(is_rtp) {
        if(auto error = srtp_protect(_session, view.ptr, view.size, view.ptr, &encrypted_size, 0)) {
            TAU_LOG_WARNING_THR(64, _log_ctx << "srtp_protect failed, error: " << error);
            return false;
        }
    } else {
        if(auto error = srtp_protect_rtcp(_session, view.ptr, view.size, view.ptr, &encrypted_size, 0)) {
            TAU_LOG_WARNING_THR(64, _log_ctx << "srtp_protect_rtcp failed, error: " << error);
            return false;
        }
    }
    packet.SetSize(encrypted_size);
    _callback(std::move(packet), is_rtp);
    return true;
}

bool Session::Decrypt(Buffer&& packet, bool is_rtp) {
    auto view = packet.GetView();
    size_t decrypted_size = view.size;
    if(is_rtp) {
        if(auto error = srtp_unprotect(_session, view.ptr, view.size, view.ptr, &decrypted_size)) {
            TAU_LOG_WARNING_THR(64, _log_ctx << "srtp_unprotect failed, error: " << error);
            return false;
        }
    } else {
        if(auto error = srtp_unprotect_rtcp(_session, view.ptr, view.size, view.ptr, &decrypted_size)) {
            TAU_LOG_WARNING_THR(64, _log_ctx << "srtp_unprotect_rtcp failed, error: " << error);
            return false;
        }
    }
    packet.SetSize(decrypted_size);
    _callback(std::move(packet), is_rtp);
    return true;
}

size_t Session::GetKeySize(srtp_profile_t profile) {
    return srtp_profile_get_master_key_length(profile);
}

size_t Session::GetSaltSize(srtp_profile_t profile) {
    return srtp_profile_get_master_salt_length(profile);
}

}
