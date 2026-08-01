#include "apps/signalling/message/Client.h"

namespace tau::signalling::message {

void ClientToJson(etl::string_stream& ss, const Client& client) {
    ss << "{";
    ss << "\"type\":\"" << client.type << "\"";
    ss << ",\"client_id\":" << client.client_id;
    ss << ",\"stream_id\":" << client.stream_id;
    if(client.session_id) {
        ss << ",\"session_id\":" << *client.session_id;
    }
    ss << ",\"payload\":";
    PayloadToJson(ss, client.payload);
    ss << "}";
}

std::optional<Client> ClientFromJson(const Json::object& parsed) {
    etl::string<16> type;
    json::GetString(parsed, "type", type);

    Client client{
        .type = TypeFromString(type),
        .client_id = json::GetUint64(parsed, "client_id"),
        .stream_id = json::GetUint64(parsed, "stream_id"),
    };
    auto session_id = json::GetUint64(parsed, "session_id");
    if(session_id) {
        client.session_id = session_id;
    }

    auto it_payload = parsed.find("payload");
    if(it_payload != parsed.end()) {
        auto payload = PayloadFromJson(it_payload->value().as_object());
        if(payload) {
            client.payload = *payload;
        }
    }
    return client;
}

etl::string_stream& operator<<(etl::string_stream& ss, const Client& client) {
    ss << "type: " << client.type;
    ss << ", client_id: " << client.client_id;
    if(client.session_id) {
        ss << ", session_id: " << *client.session_id;
    }
    ss << ", payload: " << client.payload;
    return ss;
}

}
