#include "SdpReaderWriterBase.h"

namespace tau::sdp {

class SdpWriterTest : public SdpReaderWriterBase, public ::testing::Test {
};

TEST_F(SdpWriterTest, Basic) {
    Sdp sdp{
        .cname = "rand0m-cNaMe",
        .bundle_mids = MakeBundleMids({"audio", "video", "data-42"}),
        .ice = Ice{
            .trickle = true,
            .ufrag = "test-ufrag",
            .pwd = "test-pwd",
            .candidates = MakeCandidates({
                "2651769019 1 udp 2113937151 127.0.0.1 44444 typ host"
            })
        },
        .dtls = Dtls{
            .setup = Setup::kActive,
            .fingerprint_sha256 = "00:11:22::33::44:xx"
        },
        .medias = {}
    };
    sdp.medias.push_back(Media{
        .type = MediaType::kAudio,
        .mid = "audio",
        .direction = Direction::kSendRecv,
        .codecs = MakeCodecsMap({
            {100, Codec{.index = 0, .name = "opus", .clock_rate = 48000, .rtcp_fb = 0, .format = "minptime=10;useinbandfec=1"}},
            {  0, Codec{.index = 1, .name = "PCMU", .clock_rate = 8000}},
        }),
        .ssrc = 0x12345678
    });
    sdp.medias.push_back(Media{
        .type = MediaType::kVideo,
        .mid = "video",
        .direction = Direction::kSendRecv,
        .codecs = MakeCodecsMap({
            {96, Codec{.index = 0, .name = "H264", .clock_rate = 90000, .rtcp_fb = kRtcpFbDefault, .format = "level-asymmetry-allowed=1;packetization-mode=1;profile-level-id=4d001f"}},
            {97, Codec{.index = 1, .name = "H264", .clock_rate = 90000, .rtcp_fb = kRtcpFbDefault, .format = "level-asymmetry-allowed=1;packetization-mode=1;profile-level-id=42001f"}},
            {98, Codec{.index = 2, .name = "H264", .clock_rate = 90000, .rtcp_fb = kRtcpFbDefault, .format = "level-asymmetry-allowed=1;packetization-mode=1;profile-level-id=42e01f"}},
        }),
        .ssrc = 0x9ABCDEF0
    });
    sdp.medias.push_back(Media{
        .type = MediaType::kApplication,
        .mid = "data-42",
        .direction = Direction::kSendRecv
    });
    etl::string<8192> sdp_string;
    WriteSdp(sdp_string, sdp);
    TAU_LOG_INFO("Output sdp:\n" << sdp_string);
    ASSERT_TRUE(Reader::Validate(sdp_string));

    const auto parsed_sdp = ParseSdp(sdp_string);
    ASSERT_NE(nullptr, parsed_sdp);
    ASSERT_NO_FATAL_FAILURE(AssertSdp(sdp, *parsed_sdp));
}

}
