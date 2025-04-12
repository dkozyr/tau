#include "tau/srtp/Session.h"
#include "tau/common/Exception.h"
#include "tau/common/Log.h"
#include <cstring>

namespace tau::srtp {

Session::Session(Options&& options) {
    srtp_policy_t policy;
    std::memset(&policy, 0, sizeof(srtp_policy_t));

    if(auto error = srtp_crypto_policy_set_from_profile_for_rtp(&policy.rtp, options.profile)) {
        TAU_EXCEPTION(std::runtime_error, "srtp_crypto_policy_set_from_profile_for_rtp failed, error: " << error);
    }
    if(auto error = srtp_crypto_policy_set_from_profile_for_rtcp(&policy.rtcp, options.profile)) {
        TAU_EXCEPTION(std::runtime_error, "srtp_crypto_policy_set_from_profile_for_rtcp failed, error: " << error);
    }
    policy.ssrc.type = (options.type == Type::kEncryptor)
        ? srtp_ssrc_type_t::ssrc_any_outbound
        : srtp_ssrc_type_t::ssrc_any_inbound;
    policy.key = options.key.data();
    policy.ssrc.value = 0;
    policy.window_size = kRtxWindowSize;
    policy.allow_repeat_tx = true;

    if(auto error = srtp_create(&_session, &policy)) {
        TAU_EXCEPTION(std::runtime_error, "srtp_create failed, error: " << error);
    }
}

Session::~Session() {
    srtp_dealloc(_session);
}

bool Session::Encrypt(Buffer&& packet, bool is_rtp) {
    auto view = packet.GetView();
    size_t encrypted_size = packet.GetCapacity();
    if(is_rtp) {
        if(auto error = srtp_protect(_session, view.ptr, view.size, view.ptr, &encrypted_size, 0)) {
            TAU_LOG_DEBUG("srtp_protect failed, error: " << error);
            return false;
        }
    } else {
        if(auto error = srtp_protect_rtcp(_session, view.ptr, view.size, view.ptr, &encrypted_size, 0)) {
            TAU_LOG_DEBUG("srtp_protect_rtcp failed, error: " << error);
            return false;
        }
    }
    packet.SetSize(encrypted_size);
    _callback(std::move(packet));
    return true;
}

bool Session::Decrypt(Buffer&& packet, bool is_rtp) {
    auto view = packet.GetView();
    size_t decrypted_size = view.size;
    if(is_rtp) {
        if(auto error = srtp_unprotect(_session, view.ptr, view.size, view.ptr, &decrypted_size)) {
            TAU_LOG_DEBUG("srtp_unprotect failed, error: " << error);
            return false;
        }
    } else {
        if(auto error = srtp_unprotect_rtcp(_session, view.ptr, view.size, view.ptr, &decrypted_size)) {
            TAU_LOG_DEBUG("srtp_unprotect_rtcp failed, error: " << error);
            return false;
        }
    }
    packet.SetSize(decrypted_size);
    _callback(std::move(packet));
    return true;
}

}
