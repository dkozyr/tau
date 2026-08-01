#pragma once

#include "SessionInfo.h"
#include <tau/ws/Message.h>
#include <unordered_map>
#include <cstdint>

namespace tau::signalling {

struct StreamInfo {
    DeviceId device_id;
    StreamId stream_id;

    ws::String sdp_offer; //TODO: remopve it
    uint64_t max_clients = 1;

    std::unordered_map<SessionId, SessionInfo> sessions = {};

    bool HasClientId(ClientId client_id) const {
        for(auto& [_, info] : sessions) {
            if(client_id == info.client_id) {
                return true;
            }
        }
        return false;
    }
};

}
