#include "tests/sdp/ReaderWriterBase.h"
#include "tests/sdp/SdpExamples.h"

namespace tau::sdp {

class ReaderTest :public ReaderWriterBase, public ::testing::Test {
};

TEST_F(ReaderTest, Rtsp) {
    ASSERT_TRUE(Reader::Validate(kRtspSdpExample));

    const Sdp target_sdp{
        .cname = {},
        .bundle_mids = {},
        .ice = std::nullopt,
        .dtls = std::nullopt,
        .medias = {
            Media{
                .type = MediaType::kVideo,
                .mid = {},
                .direction = Direction::kSendRecv,
                .codecs = {
                    {96, Codec{.index = 0, .name = "H264", .clock_rate = 90000, .rtcp_fb = 0, .format = "packetization-mode=1;profile-level-id=640020;sprop-parameter-sets=Z2QAIKwsqAeAIl5ZuAgICgAAAwPoAACcQQg=,aO48sA=="}},
                },
                .ssrc = std::nullopt
            }
        }
    };
    const auto parsed_sdp = ParseSdp(kRtspSdpExample);
    ASSERT_TRUE(parsed_sdp.has_value());
    ASSERT_NO_FATAL_FAILURE(AssertSdp(target_sdp, *parsed_sdp));
}

TEST_F(ReaderTest, WebrtcAudioOnly) {
    ASSERT_TRUE(Reader::Validate(kWebrtcAudioOnlySdpExample));

    const Sdp target_sdp{
        .cname = "Wg8kdYwkkqzCZqko",
        .bundle_mids = {"audio"},
        .ice = Ice{
            .trickle = false,
            .ufrag = "bzRv+Hl9e/MnTuO7",
            .pwd = "YC88frVagqjvoBpOVAd+yOCH",
            .candidates = {
                "2896278100 1 udp 2122260223 192.168.1.36 63955 typ host generation 0",
                "2896278100 2 udp 2122260222 192.168.1.36 59844 typ host generation 0",
                "3793899172 1 tcp 1518280447 192.168.1.36 0 typ host tcptype active generation 0",
                "3793899172 2 tcp 1518280446 192.168.1.36 0 typ host tcptype active generation 0",
                "1521601408 1 udp 1686052607 83.49.46.37 63955 typ srflx raddr 192.168.1.36 rport 63955 generation 0",
                "1521601408 2 udp 1686052606 83.49.46.37 59844 typ srflx raddr 192.168.1.36 rport 59844 generation 0",
            }
        },
        .dtls = Dtls{
            .setup = Setup::kActpass,
            .fingerprint_sha256 = "BE:C0:9D:93:0B:56:8C:87:48:5F:57:F7:9F:A3:D2:07:D2:8C:15:3F:DC:CE:D7:96:2B:A7:6A:DE:B8:72:F0:76"
        },
        .medias = {
            Media{
                .type = MediaType::kAudio,
                .mid = "audio",
                .direction = Direction::kSend,
                .codecs = {
                    {111, Codec{.index = 0, .name = "opus", .clock_rate = 48000, .rtcp_fb = 0, .format = "minptime=10;useinbandfec=1"}},
                    {103, Codec{.index = 1, .name = "ISAC", .clock_rate = 16000}},
                    {104, Codec{.index = 2, .name = "ISAC", .clock_rate = 32000}},
                    {  9, Codec{.index = 3, .name = "G722", .clock_rate = 8000}},
                    {  0, Codec{.index = 4, .name = "PCMU", .clock_rate = 8000}},
                    {  8, Codec{.index = 5, .name = "PCMA", .clock_rate = 8000}},
                    {106, Codec{.index = 6, .name = "CN", .clock_rate = 32000}},
                    {105, Codec{.index = 7, .name = "CN", .clock_rate = 16000}},
                    { 13, Codec{.index = 8, .name = "CN", .clock_rate = 8000}},
                    {126, Codec{.index = 9, .name = "telephone-event", .clock_rate = 8000}},
                },
                .ssrc = 655607873
            }
        }
    };
    const auto parsed_sdp = ParseSdp(kWebrtcAudioOnlySdpExample);
    ASSERT_TRUE(parsed_sdp.has_value());
    ASSERT_NO_FATAL_FAILURE(AssertSdp(target_sdp, *parsed_sdp));
}

TEST_F(ReaderTest, WebrtcChrome) {
    ASSERT_TRUE(Reader::Validate(kWebrtcChromeSdpExample));

    const Sdp target_sdp{
        .cname = "YOXhcNpX+Cu3pUyF",
        .bundle_mids = {"0", "1"},
        .ice = Ice{
            .trickle = true,
            .ufrag = "DQ5F",
            .pwd = "kj1NnowCn9vMH786My9mJzPf",
            .candidates = {
                "1145593077 1 udp 2113937151 012e3baf-5fe2-4dfd-b538-f8fbf3141e84.local 42867 typ host generation 0 network-cost 999",
                "1329961753 1 udp 1677729535 3.4.5.6 42867 typ srflx raddr 0.0.0.0 rport 0 generation 0 network-cost 999"
            }
        },
        .dtls = Dtls{
            .setup = Setup::kActive,
            .fingerprint_sha256 = "6B:E1:D7:60:D1:71:54:F5:54:95:95:09:28:E3:DF:FD:83:12:71:EA:D6:0C:D8:C2:2E:F8:CB:1C:F7:55:E1:6B"
        },
        .medias = {
            Media{
                .type = MediaType::kAudio,
                .mid = "0",
                .direction = Direction::kRecv,
                .codecs = {
                    {111, Codec{.index = 0, .name = "opus", .clock_rate = 48000, .rtcp_fb = 0, .format = "minptime=10;useinbandfec=1"}},
                    { 63, Codec{.index = 1, .name = "red", .clock_rate = 48000, .rtcp_fb = 0, .format = "111/111"}},
                    {  9, Codec{.index = 2, .name = "G722", .clock_rate = 8000}},
                    {  0, Codec{.index = 3, .name = "PCMU", .clock_rate = 8000}},
                    {  8, Codec{.index = 4, .name = "PCMA", .clock_rate = 8000}},
                    { 13, Codec{.index = 5, .name = "CN", .clock_rate = 8000}},
                    {110, Codec{.index = 6, .name = "telephone-event", .clock_rate = 48000}},
                    {126, Codec{.index = 7, .name = "telephone-event", .clock_rate = 8000}},
                },
                .ssrc = 3461839429
            },
            Media{
                .type = MediaType::kVideo,
                .mid = "1",
                .direction = Direction::kSendRecv,
                .codecs = {
                    { 96, Codec{.index = 0,  .name = "VP8",  .clock_rate = 90000, .rtcp_fb = kRtcpFbDefault, .format = {}}},
                    { 97, Codec{.index = 1,  .name = "rtx",  .clock_rate = 90000, .rtcp_fb = 0,              .format = "apt=96"}},
                    {102, Codec{.index = 2,  .name = "H264", .clock_rate = 90000, .rtcp_fb = kRtcpFbDefault, .format = "level-asymmetry-allowed=1;packetization-mode=1;profile-level-id=42001f"}},
                    {103, Codec{.index = 3,  .name = "rtx",  .clock_rate = 90000, .rtcp_fb = 0,              .format = "apt=102"}},
                    {104, Codec{.index = 4,  .name = "H264", .clock_rate = 90000, .rtcp_fb = kRtcpFbDefault, .format = "level-asymmetry-allowed=1;packetization-mode=0;profile-level-id=42001f"}},
                    {105, Codec{.index = 5,  .name = "rtx",  .clock_rate = 90000, .rtcp_fb = 0,              .format = "apt=104"}},
                    {106, Codec{.index = 6,  .name = "H264", .clock_rate = 90000, .rtcp_fb = kRtcpFbDefault, .format = "level-asymmetry-allowed=1;packetization-mode=1;profile-level-id=42e01f"}},
                    {107, Codec{.index = 7,  .name = "rtx",  .clock_rate = 90000, .rtcp_fb = 0,              .format = "apt=106"}},
                    {108, Codec{.index = 8,  .name = "H264", .clock_rate = 90000, .rtcp_fb = kRtcpFbDefault, .format = "level-asymmetry-allowed=1;packetization-mode=0;profile-level-id=42e01f"}},
                    {109, Codec{.index = 9,  .name = "rtx",  .clock_rate = 90000, .rtcp_fb = 0,              .format = "apt=108"}},
                    {127, Codec{.index = 10, .name = "H264", .clock_rate = 90000, .rtcp_fb = kRtcpFbDefault, .format = "level-asymmetry-allowed=1;packetization-mode=1;profile-level-id=4d001f"}},
                    {125, Codec{.index = 11, .name = "rtx",  .clock_rate = 90000, .rtcp_fb = 0,              .format = "apt=127"}},
                    { 39, Codec{.index = 12, .name = "H264", .clock_rate = 90000, .rtcp_fb = kRtcpFbDefault, .format = "level-asymmetry-allowed=1;packetization-mode=0;profile-level-id=4d001f"}},
                    { 40, Codec{.index = 13, .name = "rtx",  .clock_rate = 90000, .rtcp_fb = 0,              .format = "apt=39"}},
                    { 45, Codec{.index = 14, .name = "AV1",  .clock_rate = 90000, .rtcp_fb = kRtcpFbDefault, .format = "level-idx=5;profile=0;tier=0"}},
                    { 46, Codec{.index = 15, .name = "rtx",  .clock_rate = 90000, .rtcp_fb = 0,              .format = "apt=45"}},
                    { 98, Codec{.index = 16, .name = "VP9",  .clock_rate = 90000, .rtcp_fb = kRtcpFbDefault, .format = "profile-id=0"}},
                    { 99, Codec{.index = 17, .name = "rtx",  .clock_rate = 90000, .rtcp_fb = 0,              .format = "apt=98"}},
                    {100, Codec{.index = 18, .name = "VP9",  .clock_rate = 90000, .rtcp_fb = kRtcpFbDefault, .format = "profile-id=2"}},
                    {101, Codec{.index = 19, .name = "rtx",  .clock_rate = 90000, .rtcp_fb = 0,              .format = "apt=100"}},
                    {112, Codec{.index = 20, .name = "red",  .clock_rate = 90000}},
                    {113, Codec{.index = 21, .name = "rtx",  .clock_rate = 90000, .rtcp_fb = 0,              .format = "apt=112"}},
                    {114, Codec{.index = 22, .name = "ulpfec", .clock_rate = 90000}},
                },
                .ssrc = 3218536253
            },
        }
    };
    const auto parsed_sdp = ParseSdp(kWebrtcChromeSdpExample);
    ASSERT_TRUE(parsed_sdp.has_value());
    ASSERT_NO_FATAL_FAILURE(AssertSdp(target_sdp, *parsed_sdp));
}

TEST_F(ReaderTest, WebrtcSafari) {
    ASSERT_TRUE(Reader::Validate(kWebrtcSafariSdpExample));

    const Sdp target_sdp{
        .cname = "v6qP7wQYa01mcFN8",
        .bundle_mids = {"0", "1"},
        .ice = Ice{
            .trickle = true,
            .ufrag = "lGjA",
            .pwd = "KVh9jwWy5bgHIdeSRKf4yy3O",
            .candidates = {}
        },
        .dtls = Dtls{
            .setup = Setup::kActive,
            .fingerprint_sha256 = "DF:7D:1E:2D:39:BA:C5:44:DF:6F:81:D4:6D:BF:C7:0E:54:31:A3:5D:BB:BC:46:39:C0:C1:8A:69:28:AB:2F:F3"
        },
        .medias = {
            Media{
                .type = MediaType::kAudio,
                .mid = "0",
                .direction = Direction::kSendRecv,
                .codecs = {
                    {111, Codec{.index = 0, .name = "opus", .clock_rate = 48000, .rtcp_fb = 0, .format = "minptime=10;useinbandfec=1"}},
                    { 63, Codec{.index = 1, .name = "red", .clock_rate = 48000, .rtcp_fb = 0, .format = "111/111"}},
                    {  9, Codec{.index = 2, .name = "G722", .clock_rate = 8000}},
                    {  0, Codec{.index = 3, .name = "PCMU", .clock_rate = 8000}},
                    {  8, Codec{.index = 4, .name = "PCMA", .clock_rate = 8000}},
                    { 13, Codec{.index = 5, .name = "CN", .clock_rate = 8000}},
                    {110, Codec{.index = 6, .name = "telephone-event", .clock_rate = 48000}},
                    {126, Codec{.index = 7, .name = "telephone-event", .clock_rate = 8000}},
                },
                .ssrc = 616985218
            },
            Media{
                .type = MediaType::kVideo,
                .mid = "1",
                .direction = Direction::kSendRecv,
                .codecs = {
                    { 96, Codec{.index = 0,  .name = "H264", .clock_rate = 90000, .rtcp_fb = kRtcpFbDefault, .format = "level-asymmetry-allowed=1;packetization-mode=1;profile-level-id=640c1f"}},
                    { 97, Codec{.index = 1,  .name = "rtx",  .clock_rate = 90000, .rtcp_fb = 0,              .format = "apt=96"}},
                    { 98, Codec{.index = 2,  .name = "H264", .clock_rate = 90000, .rtcp_fb = kRtcpFbDefault, .format = "level-asymmetry-allowed=1;packetization-mode=1;profile-level-id=42e01f"}},
                    { 99, Codec{.index = 3,  .name = "rtx",  .clock_rate = 90000, .rtcp_fb = 0,              .format = "apt=98"}},
                    {100, Codec{.index = 4,  .name = "H264", .clock_rate = 90000, .rtcp_fb = kRtcpFbDefault, .format = "level-asymmetry-allowed=1;packetization-mode=0;profile-level-id=640c1f"}},
                    {101, Codec{.index = 5,  .name = "rtx",  .clock_rate = 90000, .rtcp_fb = 0,              .format = "apt=100"}},
                    {102, Codec{.index = 6,  .name = "H264", .clock_rate = 90000, .rtcp_fb = kRtcpFbDefault, .format = "level-asymmetry-allowed=1;packetization-mode=0;profile-level-id=42e01f"}},
                    {103, Codec{.index = 7,  .name = "rtx",  .clock_rate = 90000, .rtcp_fb = 0,              .format = "apt=102"}},
                    {104, Codec{.index = 8,  .name = "H265", .clock_rate = 90000, .rtcp_fb = kRtcpFbDefault, .format = {}}},
                    {105, Codec{.index = 9,  .name = "rtx",  .clock_rate = 90000, .rtcp_fb = 0,              .format = "apt=104"}},
                    {106, Codec{.index = 10, .name = "VP8",  .clock_rate = 90000, .rtcp_fb = kRtcpFbDefault, .format = {}}},
                    {107, Codec{.index = 11, .name = "rtx",  .clock_rate = 90000, .rtcp_fb = 0,              .format = "apt=106"}},
                    {108, Codec{.index = 12, .name = "VP9",  .clock_rate = 90000, .rtcp_fb = kRtcpFbDefault, .format = "profile-id=0"}},
                    {109, Codec{.index = 13, .name = "rtx",  .clock_rate = 90000, .rtcp_fb = 0,              .format = "apt=108"}},
                    {127, Codec{.index = 14, .name = "VP9",  .clock_rate = 90000, .rtcp_fb = kRtcpFbDefault, .format = "profile-id=2"}},
                    {125, Codec{.index = 15, .name = "rtx",  .clock_rate = 90000, .rtcp_fb = 0,              .format = "apt=127"}},
                    {112, Codec{.index = 16, .name = "red",  .clock_rate = 90000}},
                    {113, Codec{.index = 17, .name = "rtx",  .clock_rate = 90000, .rtcp_fb = 0,              .format = "apt=112"}},
                    {114, Codec{.index = 18, .name = "ulpfec", .clock_rate = 90000}},
                },
                .ssrc = 3201680545
            },
        }
    };
    const auto parsed_sdp = ParseSdp(kWebrtcSafariSdpExample);
    ASSERT_TRUE(parsed_sdp.has_value());
    ASSERT_NO_FATAL_FAILURE(AssertSdp(target_sdp, *parsed_sdp));
}

}
