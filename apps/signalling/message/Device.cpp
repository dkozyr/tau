#include "apps/signalling/message/Device.h"

namespace tau::signalling::message {

void DeviceToJson(etl::string_stream& ss, const Device& device) {
    ss << "{";
    ss << "\"type\":\"" << device.type << "\"";
    ss << ",\"device_id\":" << device.device_id;
    if(device.session_id) {
        ss << ",\"session_id\":" << *device.session_id;
    }
    ss << ",\"payload\":";
    PayloadToJson(ss, device.payload);
    ss << "}";
}

std::optional<Device> DeviceFromJson(const Json::object& parsed) {
    etl::string<16> type;
    json::GetString(parsed, "type", type);

    Device device{
        .type = TypeFromString(type),
        .device_id = json::GetUint64(parsed, "device_id"),
    };
    if(!device.device_id) {
        return std::nullopt;
    }
    auto session_id = json::GetUint64(parsed, "session_id");
    if(session_id) {
        device.session_id = session_id;
    }

    auto it_payload = parsed.find("payload");
    if(it_payload != parsed.end()) {
        auto payload = PayloadFromJson(it_payload->value().as_object());
        if(payload) {
            device.payload = *payload;
        }
    }
    return device;
}

etl::string_stream& operator<<(etl::string_stream& ss, const Device& device) {
    ss << "type: " << device.type;
    ss << ", device_id: " << device.device_id;
    if(device.session_id) {
        ss << ", session_id: " << *device.session_id;
    }
    ss << ", payload: " << device.payload;
    return ss;
}

}
