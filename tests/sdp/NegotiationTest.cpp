#include "tau/sdp/Negotiation.h"
#include "tests/sdp/SdpExamples.h"
#include "tests/lib/Common.h"

namespace tau::sdp {

class NegotiationTest : public ::testing::Test {
protected:
    const Media kDefaultRemoteAudio{
        .type = MediaType::kAudio,
        .mid = "0",
        .direction = Direction::kSend,
        .codecs = {
            { 63, Codec{.index = 1, .name = "red", .clock_rate = 48000, .rtcp_fb = 0, .format = "111/111"}},
            {  9, Codec{.index = 2, .name = "G722", .clock_rate = 8000}},
            {  0, Codec{.index = 3, .name = "PCMU", .clock_rate = 8000}},
            {  8, Codec{.index = 4, .name = "PCMA", .clock_rate = 8000}},
            { 13, Codec{.index = 5, .name = "CN", .clock_rate = 8000}},
            {111, Codec{.index = 42, .name = "opus", .clock_rate = 48000, .rtcp_fb = 0, .format = "minptime=10;useinbandfec=1"}},
            {110, Codec{.index = 6, .name = "telephone-event", .clock_rate = 48000}},
            {126, Codec{.index = 7, .name = "telephone-event", .clock_rate = 8000}},
        }
    };

    const Media kDefaultRemoteVideo{
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
            {106, Codec{.index = 6,  .name = "H264", .clock_rate = 90000, .rtcp_fb = kRtcpFbDefault, .format = "level-asymmetry-allowed=0;packetization-mode=1;profile-level-id=42e01f"}},
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
        }
    };
};

TEST_F(NegotiationTest, Audio) {
    const Media local{
        .type = MediaType::kAudio,
        .mid = "audio",
        .direction = Direction::kRecv,
        .codecs = {
            {100, Codec{.index = 0, .name = "opus", .clock_rate = 48000}},
            {  0, Codec{.index = 1, .name = "PCMU", .clock_rate = 8000}},
            {  8, Codec{.index = 2, .name = "PCMA", .clock_rate = 8000}},
        },
        .ssrc = g_random.Int<uint32_t>()
    };

    auto media = SelectMedia(kDefaultRemoteAudio, local);
    ASSERT_TRUE(media.has_value());
    ASSERT_EQ(MediaType::kAudio, media->type);
    ASSERT_EQ("0", media->mid);
    ASSERT_EQ(Direction::kRecv, media->direction);
    ASSERT_EQ(1, media->codecs.size());
    ASSERT_EQ(local.ssrc, media->ssrc);

    const auto& [pt, codec] = *media->codecs.begin();
    ASSERT_EQ(111, pt);
    ASSERT_EQ(0, codec.index);
    ASSERT_EQ("opus", codec.name);
    ASSERT_EQ(48000, codec.clock_rate);
    ASSERT_EQ(0, codec.rtcp_fb);
    ASSERT_EQ("", codec.format);
}

TEST_F(NegotiationTest, Audio2) {
    const Media local{
        .type = MediaType::kAudio,
        .mid = "audio",
        .direction = Direction::kRecv,
        .codecs = {
            {100, Codec{.index = 0, .name = "unknown", .clock_rate = 48000}},
            {111, Codec{.index = 1, .name = "nothing", .clock_rate = 8000}},
            { 88, Codec{.index = 2, .name = "PCMA", .clock_rate = 8000}},
        },
        .ssrc = g_random.Int<uint32_t>()
    };

    auto media = SelectMedia(kDefaultRemoteAudio, local);
    ASSERT_TRUE(media.has_value());
    ASSERT_EQ(MediaType::kAudio, media->type);
    ASSERT_EQ("0", media->mid);
    ASSERT_EQ(Direction::kRecv, media->direction);
    ASSERT_EQ(1, media->codecs.size());
    ASSERT_EQ(local.ssrc, media->ssrc);

    const auto& [pt, codec] = *media->codecs.begin();
    ASSERT_EQ(8, pt);
    ASSERT_EQ(0, codec.index);
    ASSERT_EQ("PCMA", codec.name);
    ASSERT_EQ(8000, codec.clock_rate);
    ASSERT_EQ(0, codec.rtcp_fb);
    ASSERT_EQ("", codec.format);
}

TEST_F(NegotiationTest, Video) {
    const Media local{
        .type = MediaType::kVideo,
        .mid = "video",
        .direction = Direction::kRecv,
        .codecs = {
            {100, Codec{.index = 0, .name = "unknown", .clock_rate = 90000}},
            {101, Codec{.index = 1, .name = "nothing", .clock_rate = 90000}},
            {102, Codec{.index = 2, .name = "H264", .clock_rate = 90000, .format = "profile-level-id=62002a"}},
            {103, Codec{.index = 3, .name = "H264", .clock_rate = 90000, .format = "profile-level-id=4d0029"}},
        },
        .ssrc = g_random.Int<uint32_t>()
    };

    auto media = SelectMedia(kDefaultRemoteVideo, local);
    ASSERT_TRUE(media.has_value());
    ASSERT_EQ(MediaType::kVideo, media->type);
    ASSERT_EQ("1", media->mid);
    ASSERT_EQ(Direction::kRecv, media->direction);
    ASSERT_EQ(1, media->codecs.size());
    ASSERT_EQ(local.ssrc, media->ssrc);

    const auto& [pt, codec] = *media->codecs.begin();
    ASSERT_EQ(127, pt);
    ASSERT_EQ(0, codec.index);
    ASSERT_EQ("H264", codec.name);
    ASSERT_EQ(90000, codec.clock_rate);
    ASSERT_EQ(kRtcpFbDefault, codec.rtcp_fb); //TODO: negotiate
    ASSERT_EQ("level-asymmetry-allowed=1;packetization-mode=1;profile-level-id=4d0029", codec.format);
}

TEST_F(NegotiationTest, FilterH264Codec_Asymmetric) {
    auto filtered = FilterH264Codec(kDefaultRemoteVideo.codecs, true);
    ASSERT_EQ(2, filtered.size());

    for(const auto& [pt, codec] : filtered) {
        ASSERT_TRUE(Contains(std::set{102, 127}, pt));
        ASSERT_EQ(std::string::npos, codec.format.find("packetization-mode=0"));
        ASSERT_NE(std::string::npos, codec.format.find("level-asymmetry-allowed=1"));
    }
}

TEST_F(NegotiationTest, FilterH264Codec_Symmetric) {
    auto filtered = FilterH264Codec(kDefaultRemoteVideo.codecs, false);
    ASSERT_EQ(3, filtered.size());

    for(const auto& [pt, codec] : filtered) {
        ASSERT_TRUE(Contains(std::set{102, 106, 127}, pt));
        ASSERT_EQ(std::string::npos, codec.format.find("packetization-mode=0"));
    }
}

TEST_F(NegotiationTest, IsH264SupportAsymmetry) {
    ASSERT_TRUE(IsH264SupportAsymmetry(kDefaultRemoteVideo.codecs));
    ASSERT_TRUE(IsH264SupportAsymmetry(CodecsMap{
        {102, Codec{.index = 2, .name = "H264", .format = "level-asymmetry-allowed=1;packetization-mode=1;profile-level-id=42001f"}},
    }));

    ASSERT_FALSE(IsH264SupportAsymmetry(CodecsMap{
        {102, Codec{.index = 2, .name = "H264", .format = "level-asymmetry-allowed=0;packetization-mode=1;profile-level-id=42001f"}},
    }));
    ASSERT_FALSE(IsH264SupportAsymmetry(CodecsMap{
        { 96, Codec{.index = 0, .name = "VP8", .format = {}}},
    }));
}

TEST_F(NegotiationTest, IsH264SameProfile) {
    ASSERT_TRUE(IsH264SameProfile ("profile-level-id=42001f", "profile-level-id=42001f"));
    ASSERT_FALSE(IsH264SameProfile("profile-level-id=42001f", "profile-level-id=42e01f"));
    ASSERT_FALSE(IsH264SameProfile("profile-level-id=42001f", "profile-level-id=4d001f"));

    ASSERT_TRUE(IsH264SameProfile ("profile-level-id=42001f", ""));
    ASSERT_FALSE(IsH264SameProfile("profile-level-id=42e01f", ""));
}

TEST_F(NegotiationTest, SelectH264Level) {
    ASSERT_EQ("1f", SelectH264Level("profile-level-id=42000a", "profile-level-id=42001f", true));
    ASSERT_EQ("1f", SelectH264Level("profile-level-id=420020", "profile-level-id=42001f", true));
    ASSERT_EQ("20", SelectH264Level("profile-level-id=42001f", "profile-level-id=420020", true));
    ASSERT_EQ("0b", SelectH264Level("profile-level-id=42000b", "profile-level-id=42001f", false));
    ASSERT_EQ("1f", SelectH264Level("profile-level-id=420020", "profile-level-id=42001f", false));
    ASSERT_EQ("0a", SelectH264Level("profile-level-id=42001f", "profile-level-id=42000a", false));

    ASSERT_EQ("1f", SelectH264Level("", "profile-level-id=42001f", true));
    ASSERT_EQ("0a", SelectH264Level("", "profile-level-id=42001f", false));
}

TEST_F(NegotiationTest, GetH264ProfileLevelId) {
    ASSERT_EQ("42001f", GetH264ProfileLevelId("level-asymmetry-allowed=1;packetization-mode=1;profile-level-id=42001f"));
    ASSERT_EQ("4d001e", GetH264ProfileLevelId("level-asymmetry-allowed=1;profile-level-id=4d001e;packetization-mode=1"));
    ASSERT_EQ("4d0020", GetH264ProfileLevelId("profile-level-id=4d0020;level-asymmetry-allowed=1;packetization-mode=1"));
    ASSERT_EQ("ANY_VA", GetH264ProfileLevelId("packetization-mode=0 profile-level-id=ANY_VALUE_WILL_BE_CUT;level-asymmetry-allowed=1"));

    ASSERT_EQ(kH264BaseProfileLevel1_0, GetH264ProfileLevelId(""));
    ASSERT_EQ(kH264BaseProfileLevel1_0, GetH264ProfileLevelId("profile-level-id=small"));
}

TEST_F(NegotiationTest, Direction) {
    ASSERT_EQ(Direction::kSendRecv, SelectDirection(Direction::kSendRecv, Direction::kSendRecv));
    ASSERT_EQ(Direction::kRecv,     SelectDirection(Direction::kSendRecv, Direction::kRecv));
    ASSERT_EQ(Direction::kSend,     SelectDirection(Direction::kSendRecv, Direction::kSend));
    ASSERT_EQ(Direction::kInactive, SelectDirection(Direction::kSendRecv, Direction::kInactive));

    ASSERT_EQ(Direction::kRecv,     SelectDirection(Direction::kSend,     Direction::kSendRecv));
    ASSERT_EQ(Direction::kRecv,     SelectDirection(Direction::kSend,     Direction::kRecv));
    ASSERT_EQ(Direction::kInactive, SelectDirection(Direction::kSend,     Direction::kSend));
    ASSERT_EQ(Direction::kInactive, SelectDirection(Direction::kSend,     Direction::kInactive));

    ASSERT_EQ(Direction::kSend,     SelectDirection(Direction::kRecv,     Direction::kSendRecv));
    ASSERT_EQ(Direction::kInactive, SelectDirection(Direction::kRecv,     Direction::kRecv));
    ASSERT_EQ(Direction::kSend,     SelectDirection(Direction::kRecv,     Direction::kSend));
    ASSERT_EQ(Direction::kInactive, SelectDirection(Direction::kRecv,     Direction::kInactive));

    ASSERT_EQ(Direction::kInactive, SelectDirection(Direction::kInactive, Direction::kSendRecv));
    ASSERT_EQ(Direction::kInactive, SelectDirection(Direction::kInactive, Direction::kRecv));
    ASSERT_EQ(Direction::kInactive, SelectDirection(Direction::kInactive, Direction::kSend));
    ASSERT_EQ(Direction::kInactive, SelectDirection(Direction::kInactive, Direction::kInactive));
}

}
