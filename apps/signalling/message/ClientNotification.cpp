#include "apps/signalling/message/ClientNotification.h"

namespace tau::signalling::message {

void ClientNotificationToJson(etl::string_stream& ss, const ClientNotification& notification) {
    ss << "{";
    ss << "\"session_id\":" << notification.session_id;
    ss << ",\"session_state\":\"" << notification.session_state << "\"";
    ss << ",\"payload\":";
    PayloadToJson(ss, notification.payload);
    ss << "}";
}

std::optional<ClientNotification> ClientNotificationFromJson(const Json::object& parsed) {
    etl::string<16> session_state;
    json::GetString(parsed, "session_state", session_state);

    ClientNotification notification{
        .session_id = json::GetUint64(parsed, "session_id"),
        .session_state = SessionStateFromString(session_state),
    };

    auto it_payload = parsed.find("payload");
    if(it_payload != parsed.end()) {
        auto payload = PayloadFromJson(it_payload->value().as_object());
        if(payload) {
            notification.payload = *payload;
        }
    }
    return notification;
}

etl::string_stream& operator<<(etl::string_stream& ss, const ClientNotification& notification) {
    ss << "session_id: " << notification.session_id;
    ss << ", session_state: " << notification.session_state;
    ss << ", payload: " << notification.payload;
    return ss;
}

}
