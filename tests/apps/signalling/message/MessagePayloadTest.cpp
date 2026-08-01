#include "apps/signalling/message/Payload.h"
#include "tests/lib/Common.h"

namespace tau::signalling::message {

TEST(MessagePayloadTest, FromString) {
    ASSERT_EQ(PayloadTypeFromString("empty"), PayloadType::kEmpty);
    ASSERT_EQ(PayloadTypeFromString("sdp"), PayloadType::kSdp);
    ASSERT_EQ(PayloadTypeFromString("ice_candidates"), PayloadType::kIceCandidates);
    ASSERT_EQ(PayloadTypeFromString("wrong_value"), PayloadType::kError);
}

TEST(MessagePayloadTest, OutputOperator) {
    etl::string<64> output;
    etl::string_stream ss{output};
    ss << PayloadType::kEmpty << ", " << PayloadType::kSdp << ", " << PayloadType::kIceCandidates << ", " << PayloadType::kError;
    ASSERT_EQ(output, "empty, sdp, ice_candidates, error");
}

TEST(MessagePayloadTest, ToJson) {
    etl::string<64> output;
    etl::string_stream ss{output};
    Payload payload{
        .type = PayloadType::kSdp,
        .data = "SDP answer"
    };
    TAU_LOG_INFO("Payload: " << payload);

    PayloadToJson(ss, payload);
    ASSERT_EQ(output, R"({"type":"sdp","data":"SDP answer"})");
}

TEST(MessagePayloadTest, FromJson) {
    etl::string<64> message_json = R"({"payload":{"type":"sdp","data":"SDP offer"}})";

    boost_ec ec;
    auto parsed = Json::parse(message_json.data(), ec);
    ASSERT_EQ(0, ec.value());

    auto payload = PayloadFromJson(parsed.at("payload").as_object());
    ASSERT_TRUE(payload.has_value());
    ASSERT_EQ(payload->type, PayloadType::kSdp);
    ASSERT_EQ(payload->data, "SDP offer");
}

}
