#include "apps/signalling/message/DeviceNotification.h"

namespace tau::signalling::message {

void DeviceNotificationToJson(etl::string_stream& ss, const DeviceNotification& notification) {
    ss << "{";
    ss << "\"stream_id\":" << notification.stream_id;
    if(notification.session_id) {
        ss << ",\"session_id\":" << *notification.session_id;
    }
    if(notification.session_state) {
        ss << ",\"session_state\":\"" << *notification.session_state << "\"";
    }
    ss << ",\"payload\":";
    PayloadToJson(ss, notification.payload);
    ss << "}";
}

std::optional<DeviceNotification> DeviceNotificationFromJson(const Json::object& parsed) {
    DeviceNotification notification{
        .stream_id = json::GetUint64(parsed, "stream_id"),
    };
    if(!notification.stream_id) {
        return std::nullopt;
    }

    const auto session_id = json::GetUint64(parsed, "session_id");
    if(session_id) {
        notification.session_id = session_id;
    }

    etl::string<16> session_state;
    json::GetString(parsed, "session_state", session_state);
    if(!session_state.empty()) {
        notification.session_state = SessionStateFromString(session_state);
    }

    auto it_payload = parsed.find("payload");
    if(it_payload != parsed.end()) {
        auto payload = PayloadFromJson(it_payload->value().as_object());
        if(payload) {
            notification.payload = *payload;
        }
    }
    return notification;
}

etl::string_stream& operator<<(etl::string_stream& ss, const DeviceNotification& notification) {
    ss << "stream_id: " << notification.stream_id;
    if(notification.session_id) {
        ss << ", session_id: " << *notification.session_id;
    }
    if(notification.session_state) {
        ss << ", session_state: " << *notification.session_state;
    }
    ss << ", payload: " << notification.payload;
    return ss;
}

}
