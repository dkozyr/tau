#pragma once

#include "apps/signalling/SessionState.h"
#include "apps/signalling/message/Type.h"
#include "apps/signalling/message/Payload.h"
#include "tau/common/Json.h"
#include <optional>

namespace tau::signalling::message {

struct Client {
    Type type;
    ClientId client_id;
    StreamId stream_id;
    std::optional<SessionId> session_id = std::nullopt;
    Payload payload = {};
};

void ClientToJson(etl::string_stream& ss, const Client& client);
std::optional<Client> ClientFromJson(const Json::object& parsed);

etl::string_stream& operator<<(etl::string_stream& ss, const Client& message);

}
