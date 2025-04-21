#include "tests/sdp/ReaderWriterBase.h"

namespace tau::sdp {

class WriterTest : public ReaderWriterBase, public ::testing::Test {
};

TEST_F(WriterTest, Basic) {
    const Sdp sdp{
        .bundle_mids = {"42"},
        .ice = Ice{
            .trickle = true,
            .ufrag = "test-ufrag",
            .pwd = "test-pwd",
            .candidates = {
                "2651769019 1 udp 2113937151 127.0.0.1 44444 typ host"
            }
        },
        .dtls = Dtls{
            .setup = Setup::kActive,
            .fingerprint_sha256 = "00:11:22::33::44:xx"
        },
        .medias = {
            Media{
                .type = MediaType::kApplication,
                .mid = "42",
                .direction = Direction::kSendRecv
            }
        }
    };
    auto sdp_string = WriteSdp(sdp);
    TAU_LOG_INFO("Output sdp:\n" << sdp_string);
    ASSERT_TRUE(Reader::Validate(sdp_string));

    const auto parsed_sdp = ParseSdp(sdp_string);
    ASSERT_TRUE(parsed_sdp.has_value());
    ASSERT_NO_FATAL_FAILURE(AssertSdp(sdp, *parsed_sdp));
}

}
