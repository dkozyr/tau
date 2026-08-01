#include "apps/signalling/message/Client.h"
#include "apps/signalling/message/ClientNotification.h"
#include "tests/lib/Common.h"

namespace tau::signalling::message {

TEST(MessageClientTest, ToJson) {
    Client client{
        .type = Type::kInit,
        .client_id = 12345,
        .stream_id = 67890,
        .payload = Payload{
            .type = PayloadType::kSdp,
            .data = "example 42"
        }
    };
    TAU_LOG_INFO("Client message: " << client);

    etl::string<128> output;
    etl::string_stream ss{output};
    ClientToJson(ss, client);
    TAU_LOG_INFO("Client JSON: " << output);
    ASSERT_EQ(output, R"({"type":"init","client_id":12345,"stream_id":67890,"payload":{"type":"sdp","data":"example 42"}})");
}

TEST(MessageClientTest, FromJson) {
    etl::string<256> message_json = R"({"type":"ice_candidates","client_id":54321,"stream_id":998877,"session_id":4242,"payload":{"type":"ice_candidates","data":"some data here"}})";

    boost_ec ec;
    auto parsed = Json::parse(message_json.data(), ec);
    ASSERT_EQ(0, ec.value());

    auto client = ClientFromJson(parsed.as_object());
    ASSERT_TRUE(client.has_value());
    ASSERT_EQ(client->type, Type::kIceCandidates);
    ASSERT_EQ(client->client_id, 54321);
    ASSERT_EQ(client->stream_id, 998877);
    ASSERT_EQ(client->session_id, 4242);
    ASSERT_EQ(client->payload.type, PayloadType::kIceCandidates);
    ASSERT_EQ(client->payload.data, "some data here");
    TAU_LOG_INFO("Parsed client: " << *client);
}

TEST(MessageClientTest, ClientNotification_ToJson) {
    ClientNotification notification{
        .session_id = 192837465,
        .session_state = SessionState::kSdpAnswered,
        .payload = Payload{
            .type = PayloadType::kEmpty,
            .data = "payload data"
        }
    };
    TAU_LOG_INFO("ClientNotification: " << notification);

    etl::string<256> output;
    etl::string_stream ss{output};
    ClientNotificationToJson(ss, notification);
    TAU_LOG_INFO("ClientNotification json: " << output);
    ASSERT_EQ(output, R"({"session_id":192837465,"session_state":"sdp_answered","payload":{"type":"empty","data":"payload data"}})");

}

TEST(MessageClientTest, ClientNotification_FromJson) {
    etl::string<256> message_json = R"({"session_id":192837465,"session_state":"streaming","payload":{"type":"empty","data":"payload data"}})";

    boost_ec ec;
    auto parsed = Json::parse(message_json.data(), ec);
    ASSERT_EQ(0, ec.value());

    auto notification = ClientNotificationFromJson(parsed.as_object());
    ASSERT_TRUE(notification.has_value());
    ASSERT_EQ(notification->session_id, 192837465);
    ASSERT_EQ(notification->session_state, SessionState::kStreaming);
    ASSERT_EQ(notification->payload.type, PayloadType::kEmpty);
    ASSERT_EQ(notification->payload.data, "payload data");
    TAU_LOG_INFO("ClientNotification: " << *notification);
}

}
