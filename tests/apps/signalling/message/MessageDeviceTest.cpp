#include "apps/signalling/message/Device.h"
#include "apps/signalling/message/DeviceNotification.h"
#include "tests/lib/Common.h"

namespace tau::signalling::message {

TEST(MessageDeviceTest, ToJson) {
    Device device{
        .type = Type::kInit,
        .device_id = 12345,
        .session_id = 67890,
        .payload = Payload{
            .type = PayloadType::kSdp,
            .data = "example 42"
        }
    };
    TAU_LOG_INFO("Device message: " << device);

    etl::string<128> output;
    etl::string_stream ss{output};
    DeviceToJson(ss, device);
    TAU_LOG_INFO("Device JSON: " << output);
    ASSERT_EQ(output, R"({"type":"init","device_id":12345,"session_id":67890,"payload":{"type":"sdp","data":"example 42"}})");
}

TEST(MessageDeviceTest, FromJson) {
    etl::string<256> message_json = R"({"type":"ice_candidates","device_id":54321,"session_id":998877,"payload":{"type":"ice_candidates","data":"some data here"}})";

    boost_ec ec;
    auto parsed = Json::parse(message_json.data(), ec);
    ASSERT_EQ(0, ec.value());

    auto device = DeviceFromJson(parsed.as_object());
    ASSERT_TRUE(device.has_value());
    ASSERT_EQ(device->type, Type::kIceCandidates);
    ASSERT_EQ(device->device_id, 54321);
    ASSERT_EQ(device->session_id, 998877);
    ASSERT_EQ(device->payload.type, PayloadType::kIceCandidates);
    ASSERT_EQ(device->payload.data, "some data here");
    TAU_LOG_INFO("Parsed device: " << *device);
}

TEST(MessageDeviceTest, FromJson_WithoutSessionId) {
    etl::string<256> message_json = R"({"type":"sdp","device_id":54321,"payload":{"type":"sdp","data":"sdp-42"}})";

    boost_ec ec;
    auto parsed = Json::parse(message_json.data(), ec);
    ASSERT_EQ(0, ec.value());

    auto device = DeviceFromJson(parsed.as_object());
    ASSERT_TRUE(device.has_value());
    ASSERT_EQ(device->type, Type::kSdp);
    ASSERT_EQ(device->device_id, 54321);
    ASSERT_FALSE(device->session_id.has_value());
    ASSERT_EQ(device->payload.type, PayloadType::kSdp);
    ASSERT_EQ(device->payload.data, "sdp-42");
    TAU_LOG_INFO("Parsed device: " << *device);
}

TEST(MessageDeviceTest, FromJson_WithoutPayload) {
    etl::string<256> message_json = R"({"type":"sdp","device_id":54321})";

    boost_ec ec;
    auto parsed = Json::parse(message_json.data(), ec);
    ASSERT_EQ(0, ec.value());

    auto device = DeviceFromJson(parsed.as_object());
    ASSERT_TRUE(device.has_value());
    ASSERT_EQ(device->type, Type::kSdp);
    ASSERT_EQ(device->device_id, 54321);
    ASSERT_FALSE(device->session_id.has_value());
    ASSERT_EQ(device->payload.type, PayloadType::kEmpty);
    ASSERT_TRUE(device->payload.data.empty());
    TAU_LOG_INFO("Parsed device: " << *device);
}

TEST(MessageDeviceTest, FromJson_WithoutType) {
    etl::string<256> message_json = R"({"device_id":54321,"session_id":42})";

    boost_ec ec;
    auto parsed = Json::parse(message_json.data(), ec);
    ASSERT_EQ(0, ec.value());

    auto device = DeviceFromJson(parsed.as_object());
    ASSERT_TRUE(device.has_value());
    ASSERT_EQ(device->type, Type::kError);
    ASSERT_EQ(device->device_id, 54321);
    ASSERT_EQ(device->session_id, 42);
    ASSERT_EQ(device->payload.type, PayloadType::kEmpty);
    ASSERT_TRUE(device->payload.data.empty());
    TAU_LOG_INFO("Parsed device: " << *device);
}

TEST(MessageDeviceTest, DeviceNotification_ToJson) {
    DeviceNotification notification{
        .stream_id = 12345,
        .session_id = 1002,
        .session_state = SessionState::kSdpOffered,
        .payload = Payload{
            .type = PayloadType::kSdp,
            .data = "SDP answer"
        }
    };
    TAU_LOG_INFO("DeviceNotification: " << notification);

    etl::string<256> output;
    etl::string_stream ss{output};
    DeviceNotificationToJson(ss, notification);
    TAU_LOG_INFO("DeviceNotification json: " << output);
    ASSERT_EQ(output, R"({"stream_id":12345,"session_id":1002,"session_state":"sdp_offered","payload":{"type":"sdp","data":"SDP answer"}})");
}

TEST(MessageDeviceTest, DeviceNotification_FromJson) {
    etl::string<256> message_json = R"({"stream_id":12345,"session_id":7654,"session_state":"sdp_answered","payload":{"type":"ice_candidates","data":"some data here"}})";

    boost_ec ec;
    auto parsed = Json::parse(message_json.data(), ec);
    ASSERT_EQ(0, ec.value());

    auto notification = DeviceNotificationFromJson(parsed.as_object());
    ASSERT_TRUE(notification.has_value());
    ASSERT_EQ(notification->stream_id, 12345);
    ASSERT_TRUE(notification->session_id.has_value());
    ASSERT_EQ(notification->session_id, 7654);
    ASSERT_EQ(notification->session_state, SessionState::kSdpAnswered);
    ASSERT_EQ(notification->payload.type, PayloadType::kIceCandidates);
    ASSERT_EQ(notification->payload.data, "some data here");
    TAU_LOG_INFO("Device notification: " << *notification);
}

TEST(MessageDeviceTest, DeviceNotification_FromJsonWithoutSessionId) {
    etl::string<256> message_json = R"({"stream_id":12345,"session_state":"sdp_answered","payload":{"type":"ice_candidates","data":"some data here"}})";

    boost_ec ec;
    auto parsed = Json::parse(message_json.data(), ec);
    ASSERT_EQ(0, ec.value());

    auto notification = DeviceNotificationFromJson(parsed.as_object());
    ASSERT_TRUE(notification.has_value());
    ASSERT_EQ(notification->stream_id, 12345);
    ASSERT_FALSE(notification->session_id.has_value());
    ASSERT_EQ(notification->session_state, SessionState::kSdpAnswered);
    ASSERT_EQ(notification->payload.type, PayloadType::kIceCandidates);
    ASSERT_EQ(notification->payload.data, "some data here");
    TAU_LOG_INFO("Device notification: " << *notification);
}

}
