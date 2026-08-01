#include "apps/signalling/Storage.h"
#include "tests/lib/Common.h"

namespace tau::signalling {

using namespace message;

class StorageTest : public ::testing::Test {
public:
    using String = ws::String;

    struct SdpAnswer {
        DeviceId device_id;
        SessionId session_id;
        String sdp;
    };
    struct IceCandidates {
        DeviceId device_id;
        SessionId session_id;
        String ice_candidates;
    };

    struct Message {
        DeviceId device_id;
        SessionId session_id;
        SessionState session_state;
        Payload payload;
    };

    StorageTest() {
        _storage.SetDeviceMessageCallback(
            [this](DeviceId device_id, SessionId session_id, SessionState session_state, Payload&& payload) {
                TAU_LOG_INFO("[Device] device_id: " << device_id << ", session_id: " << session_id << ", session_state: " << session_state << ", payload: " << payload.type);
                _device_messages.push_back({device_id, session_id, session_state, std::move(payload)});
            });
        _storage.SetClientMessageCallback(
            [this](DeviceId device_id, SessionId session_id, SessionState session_state, Payload&& payload) {
                TAU_LOG_INFO("[Client] device_id: " << device_id << ", session_id: " << session_id << ", session_state: " << session_state << ", payload: " << payload.type);
                _client_messages.push_back({device_id, session_id, session_state, std::move(payload)});
            });
    }

protected:
    StreamId InitDeviceAndGetStreamId() {
        return InitDeviceAndGetStreamId(GenerateDeviceId());
    }

    StreamId InitDeviceAndGetStreamId(DeviceId device_id) {
        auto device_init = _storage.ProcessMessage(CreateDeviceInitMessage(device_id));
        EXPECT_NO_FATAL_FAILURE(AssertValidateResult(device_init, std::nullopt));
        return device_init.stream_id;
    }

    static String MakeMessage(const Json::object& object) {
        ws::String message;
        json::Serialize(object, message);
        return message;
    }

    static Device CreateDeviceInitMessage(DeviceId device_id) {
        return Device{
            .type = Type::kInit,
            .device_id = device_id,
            .session_id = std::nullopt,
            .payload = Payload{}
        };
    }

    static Device CreateDeviceSdpMessage(DeviceId device_id, SessionId session_id) {
        return Device{
            .type = Type::kSdp,
            .device_id = device_id,
            .session_id = session_id,
            .payload = Payload{
                .type = PayloadType::kSdp,
                .data = "SDP offer"
            }
        };
    }

    static Device CreateDeviceIceMessage(DeviceId device_id, SessionId session_id) {
        return Device{
            .type = Type::kIceCandidates,
            .device_id = device_id,
            .session_id = session_id,
            .payload = Payload{
                .type = PayloadType::kIceCandidates,
                .data = "[device] ICE candidate"
            }
        };
    }

    static Device CreateDeviceCloseMessage(DeviceId device_id) {
        return Device{
            .type = Type::kClose,
            .device_id = device_id,
            .session_id = std::nullopt,
            .payload = Payload{}
        };
    }

    static Client CreateClientInitMessage(ClientId client_id, StreamId stream_id) {
        return Client{
            .type = Type::kInit,
            .client_id = client_id,
            .stream_id = stream_id,
            .session_id = std::nullopt,
            .payload = Payload{}
        };
    }

    static Client CreateClientCloseMessage(ClientId client_id, StreamId stream_id, SessionId session_id) {
        return Client{
            .type = Type::kClose,
            .client_id = client_id,
            .stream_id = stream_id,
            .session_id = session_id,
            .payload = Payload{}
        };
    }

    static Client CreateClientSdpMessage(ClientId client_id, StreamId stream_id, SessionId session_id) {
        return Client{
            .type = Type::kSdp,
            .client_id = client_id,
            .stream_id = stream_id,
            .session_id = session_id,
            .payload = Payload{
                .type = PayloadType::kSdp,
                .data = "SDP answer"
            }
        };
    }

    static Client CreateClientIceMessage(ClientId client_id, StreamId stream_id, SessionId session_id) {
        return Client{
            .type = Type::kIceCandidates,
            .client_id = client_id,
            .stream_id = stream_id,
            .session_id = session_id,
            .payload = Payload{
                .type = PayloadType::kIceCandidates,
                .data = "[client] ICE candidate"
            }
        };
    }

    static void AssertValidateResult(const DeviceNotification& notification, std::optional<SessionState> target_state, std::optional<StreamId> target_stream_id = std::nullopt) {
        ASSERT_NE(0, notification.stream_id);
        if(target_stream_id) {
            ASSERT_EQ(*target_stream_id, notification.stream_id);
        }
        ASSERT_EQ(target_state, notification.session_state);
        ASSERT_NE(PayloadType::kError, notification.payload.type);
    }

    static void AssertValidateResult(const ClientNotification& notification, SessionState target_state, std::optional<StreamId> target_session_id = std::nullopt) {
        ASSERT_NE(0, notification.session_id);
        if(target_session_id) {
            ASSERT_EQ(*target_session_id, notification.session_id);
        }
        ASSERT_EQ(target_state, notification.session_state);
        ASSERT_NE(PayloadType::kError, notification.payload.type);
    }

    static void AssertValidateErrorResult(const DeviceNotification& notification, const String& error) {
        ASSERT_EQ(0, notification.stream_id);
        ASSERT_EQ(PayloadType::kError, notification.payload.type);
        ASSERT_EQ(error, notification.payload.data);
    }

    static void AssertValidateErrorResult(const ClientNotification& notification, const String& error) {
        ASSERT_EQ(0, notification.session_id);
        ASSERT_EQ(SessionState::kUnknown, notification.session_state);
        ASSERT_EQ(PayloadType::kError, notification.payload.type);
        ASSERT_EQ(error, notification.payload.data);
    }

    static void AssertMessage(const Message& expected, const Message& actual) {
        ASSERT_EQ(expected.device_id,     actual.device_id);
        ASSERT_EQ(expected.session_id,    actual.session_id);
        ASSERT_EQ(expected.session_state, actual.session_state);
        ASSERT_EQ(expected.payload.type,  actual.payload.type);
        ASSERT_EQ(expected.payload.data,  actual.payload.data);
    }

    static uint64_t GenerateDeviceId() {
        return g_random.Int<uint64_t>(1'000'000'000, 10'000'000'000);
    }

    static uint64_t GenerateClientId() {
        return g_random.Int<uint64_t>(1'000'000'000, 10'000'000'000);
    }

protected:
    Storage _storage;

    std::vector<Message> _device_messages;
    std::vector<Message> _client_messages;
};

TEST_F(StorageTest, ClientSdpAndIce) {
    const auto device_id = GenerateDeviceId();
    const auto stream_id = InitDeviceAndGetStreamId(device_id);
    const auto client_id = GenerateClientId();

    auto client_init_notification = _storage.ProcessMessage(CreateClientInitMessage(client_id, stream_id));
    ASSERT_NO_FATAL_FAILURE(AssertValidateResult(client_init_notification, SessionState::kWait));
    const auto session_id = client_init_notification.session_id;
    ASSERT_EQ(1, _device_messages.size());
    ASSERT_NO_FATAL_FAILURE(AssertMessage(_device_messages[0], {device_id, session_id, SessionState::kWait, Payload{PayloadType::kEmpty, {}}}));

    auto device_sdp_notification = _storage.ProcessMessage(CreateDeviceSdpMessage(device_id, session_id));
    TAU_LOG_INFO("device_sdp_notification: " << device_sdp_notification);
    ASSERT_NO_FATAL_FAILURE(AssertValidateResult(device_sdp_notification, SessionState::kSdpOffered));
    ASSERT_EQ(1, _client_messages.size());
    ASSERT_NO_FATAL_FAILURE(AssertMessage(_client_messages[0], {device_id, session_id, SessionState::kSdpOffered, Payload{PayloadType::kSdp, "SDP offer"}}));

    auto device_second_sdp_notification = _storage.ProcessMessage(CreateDeviceSdpMessage(device_id, session_id));
    TAU_LOG_INFO("device_second_sdp_notification: " << device_second_sdp_notification);
    ASSERT_NO_FATAL_FAILURE(AssertValidateErrorResult(device_second_sdp_notification, "Wrong session state"));

    auto client_sdp_notification = _storage.ProcessMessage(CreateClientSdpMessage(client_id, stream_id, session_id));
    TAU_LOG_INFO("client_sdp_notification: " << client_sdp_notification);
    ASSERT_NO_FATAL_FAILURE(AssertValidateResult(client_sdp_notification, SessionState::kStreaming));
    ASSERT_EQ(2, _device_messages.size());
    ASSERT_NO_FATAL_FAILURE(AssertMessage(_device_messages[1], {device_id, session_id, SessionState::kStreaming, Payload{PayloadType::kSdp, "SDP answer"}}));

    auto client_second_sdp_notification = _storage.ProcessMessage(CreateClientSdpMessage(client_id, stream_id, session_id));
    TAU_LOG_INFO("client_second_sdp_notification: " << client_second_sdp_notification);
    ASSERT_NO_FATAL_FAILURE(AssertValidateErrorResult(client_second_sdp_notification, "Wrong session state"));

    auto client_ice_notification = _storage.ProcessMessage(CreateClientIceMessage(client_id, stream_id, session_id));
    TAU_LOG_INFO("client_ice_notification: " << client_ice_notification);
    ASSERT_NO_FATAL_FAILURE(AssertValidateResult(client_ice_notification, SessionState::kStreaming));
    ASSERT_EQ(3, _device_messages.size());
    ASSERT_NO_FATAL_FAILURE(AssertMessage(_device_messages[2], {device_id, session_id, SessionState::kStreaming, Payload{PayloadType::kIceCandidates, "[client] ICE candidate"}}));

    auto device_ice_notification = _storage.ProcessMessage(CreateDeviceIceMessage(device_id, session_id));
    TAU_LOG_INFO("device_ice_notification: " << device_ice_notification);
    ASSERT_NO_FATAL_FAILURE(AssertValidateResult(device_ice_notification, SessionState::kStreaming));
    ASSERT_EQ(2, _client_messages.size());
    ASSERT_NO_FATAL_FAILURE(AssertMessage(_client_messages[1], {device_id, session_id, SessionState::kStreaming, Payload{PayloadType::kIceCandidates, "[device] ICE candidate"}}));

    auto client_close_notification = _storage.ProcessMessage(CreateClientCloseMessage(client_id, stream_id, session_id));
    TAU_LOG_INFO("client_close_notification: " << client_close_notification);
    ASSERT_NO_FATAL_FAILURE(AssertValidateResult(client_close_notification, SessionState::kClosed));
    ASSERT_EQ(4, _device_messages.size());
    ASSERT_NO_FATAL_FAILURE(AssertMessage(_device_messages[3], {device_id, session_id, SessionState::kClosed, Payload{PayloadType::kEmpty, {}}}));
}

}
