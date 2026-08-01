#pragma once

#include "apps/signalling/SessionState.h"
#include "apps/signalling/message/Type.h"
#include "apps/signalling/message/Payload.h"
#include "tau/common/Json.h"
#include <optional>

namespace tau::signalling::message {

struct ClientNotification {
    SessionId session_id;
    SessionState session_state;
    Payload payload = {};
};

void ClientNotificationToJson(etl::string_stream& ss, const ClientNotification& notification);
std::optional<ClientNotification> ClientNotificationFromJson(const Json::object& parsed);

etl::string_stream& operator<<(etl::string_stream& ss, const ClientNotification& notification);

}
