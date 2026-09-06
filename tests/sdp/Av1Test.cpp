#include "ReaderWriterBase.h"
#include "SdpExamples.h"
#include <tau/sdp/Negotiation.h>

namespace tau::sdp {

class Av1Test : public ReaderWriterBase, public ::testing::Test {
public:
    static constexpr uint8_t kDefaultPt = 96;

protected:
    static Media MakeMedia(etl::string_view format = {}, uint8_t payload_type = kDefaultPt) {
        return Media{
            .type = MediaType::kVideo,
            .mid = "video",
            .direction = Direction::kSendRecv,
            .codecs = MakeCodecsMap({
                {payload_type, Codec{.index = 0, .name = "AV1", .clock_rate = 90000, .rtcp_fb = kRtcpFbDefault, .format = Codec::Format(format)}}
            })
        };
    }
};

TEST_F(Av1Test, FormatDefaults) {
    const auto format = ParseAv1Format("");
    ASSERT_TRUE(format);
    EXPECT_EQ(0, format->profile);
    EXPECT_EQ(5, format->level_index);
    EXPECT_EQ(0, format->tier);
    EXPECT_EQ("profile=0;level-idx=5;tier=0", CreateAv1Format(*format));
    EXPECT_EQ(5, ParseAv1Format("profile=2")->level_index);
    EXPECT_EQ(0, ParseAv1Format("level-idx=8")->tier);
}

TEST_F(Av1Test, FormatWhitespaceAndUnknownParameters) {
    const auto format = ParseAv1Format(" x-profile=99; PROFILE = 2 ; tier=1; level-idx = 12 ; ignored=garbage;");
    ASSERT_TRUE(format);
    EXPECT_EQ(2, format->profile);
    EXPECT_EQ(12, format->level_index);
    EXPECT_EQ(1, format->tier);
    EXPECT_EQ("profile=2;level-idx=12;tier=1", CreateAv1Format(*format));
}

TEST_F(Av1Test, InvalidFormats) {
    EXPECT_TRUE(ParseAv1Format("profile=2;level-idx=23;tier=1"));
    EXPECT_TRUE(ParseAv1Format("level-idx=31;tier=1"));
    EXPECT_TRUE(ParseAv1Format("level-idx=0;tier=0"));
}

TEST_F(Av1Test, ProfileMatching) {
    EXPECT_TRUE(IsAv1SameProfile("", "profile=0;level-idx=12;tier=1"));
    EXPECT_TRUE(IsAv1SameProfile("profile=2;level-idx=8;tier=1", "profile=2"));
    EXPECT_TRUE(IsAv1SameProfile("profile=invalid", "profile=invalid"));
    EXPECT_FALSE(IsAv1SameProfile("profile=1", "profile=2"));
    EXPECT_FALSE(IsAv1SameProfile("profile=1", ""));
}

TEST_F(Av1Test, Filter) {
    const auto codecs = MakeCodecsMap({
        {96, Codec{.index = 0, .name = "AV1", .clock_rate = 90000}},
        {97, Codec{.index = 1, .name = "av1", .clock_rate = 90000, .format = "profile=9"}},
        {98, Codec{.index = 2, .name = "AV1", .clock_rate = 8000}},
        {99, Codec{.index = 3, .name = "AV1", .clock_rate = 90000, .format = "profile=2"}},
        {100, Codec{.index = 4, .name = "H264", .clock_rate = 90000}},
        {101, Codec{.index = 5, .name = "rtx", .clock_rate = 90000, .format = "apt=96"}}
    });
    const auto filtered = FilterAv1Codec(codecs);
    ASSERT_EQ(2, filtered.size());

    EXPECT_EQ(0, filtered.at(96).index);
    EXPECT_EQ("", filtered.at(96).format);

    EXPECT_EQ(3, filtered.at(99).index);
    EXPECT_EQ("profile=2", filtered.at(99).format);
}

TEST_F(Av1Test, NegotiateDefaultsAndMetadata) {
    auto remote = MakeMedia("", 45);
    remote.mid = "1";
    remote.direction = Direction::kSend;

    auto local = MakeMedia("profile=0;level-idx=5;tier=0", 120);
    local.direction = Direction::kRecv;
    local.ssrc = 1234;
    local.codecs.at(120).rtcp_fb = RtcpFb::kNack | RtcpFb::kPli;

    const auto selected = SelectMedia(remote, local);
    ASSERT_TRUE(selected);
    EXPECT_EQ("1", selected->mid);
    EXPECT_EQ(Direction::kRecv, selected->direction);
    EXPECT_EQ(local.ssrc, selected->ssrc);
    ASSERT_EQ(1, selected->codecs.size());

    const auto& codec = selected->codecs.at(45);
    EXPECT_EQ("AV1", codec.name);
    EXPECT_EQ(90000, codec.clock_rate);
    EXPECT_EQ(0, codec.index);
    EXPECT_EQ(RtcpFb::kNack | RtcpFb::kPli, codec.rtcp_fb);
    EXPECT_EQ("profile=0;level-idx=5;tier=0", codec.format);
}

TEST_F(Av1Test, AsymmetricLevelAndTier) {
    const auto remote = MakeMedia("profile=0;level-idx=5;tier=0");
    const auto local  = MakeMedia("profile=0;level-idx=12;tier=1");

    const auto answer = SelectMedia(remote, local);
    ASSERT_TRUE(answer);
    ASSERT_EQ(1, answer->codecs.size());
    EXPECT_EQ("profile=0;level-idx=12;tier=1", answer->codecs.at(kDefaultPt).format);

    const auto reverse = SelectMedia(local, remote);
    ASSERT_TRUE(reverse);
    ASSERT_EQ(1, reverse->codecs.size());
    EXPECT_EQ("profile=0;level-idx=5;tier=0", reverse->codecs.at(kDefaultPt).format);
}

TEST_F(Av1Test, UnsupportedProfile) {
    const auto local  = MakeMedia("profile=0");
    const auto remote = MakeMedia("profile=1");
    ASSERT_TRUE(SelectMedia(remote, local)->codecs.empty());
}

TEST_F(Av1Test, InvalidClockRate) {
    const auto local  = MakeMedia("profile=0");
    auto remote = MakeMedia("profile=1");
    remote.codecs.at(kDefaultPt).format = "";
    remote.codecs.at(kDefaultPt).clock_rate = 8000;
    ASSERT_TRUE(SelectMedia(remote, local)->codecs.empty());
}

TEST_F(Av1Test, LocalPriorityAndRemotePayloadOrder) {
    auto local = MakeMedia("profile=1", 100);
    local.codecs.at(100).index = 2;
    local.codecs.insert({101, Codec{.index = 0, .name = "av1", .clock_rate = 90000, .format = "profile=2"}});

    auto remote = MakeMedia("profile=1", 45);
    remote.codecs.insert({47, Codec{.index = 1, .name = "AV1", .clock_rate = 90000, .format = "profile=2"}});
    remote.codecs.insert({46, Codec{.index = 2, .name = "AV1", .clock_rate = 90000, .format = "profile=2"}});

    const auto selected = SelectMedia(remote, local);
    ASSERT_TRUE(selected);
    ASSERT_EQ(1, selected->codecs.size());
    EXPECT_EQ("AV1", selected->codecs.at(47).name);
    EXPECT_EQ("profile=2;level-idx=5;tier=0", selected->codecs.at(47).format);

    remote.codecs.erase(46);
    remote.codecs.erase(47);

    const auto fallback = SelectMedia(remote, local);
    ASSERT_TRUE(fallback);
    ASSERT_EQ(1, fallback->codecs.size());
    EXPECT_EQ("profile=1;level-idx=5;tier=0", fallback->codecs.at(45).format);
}

TEST_F(Av1Test, CodecPreference) {
    auto remote = MakeMedia();
    auto local = MakeMedia();

    remote.codecs.insert({97, Codec{.index = 1, .name = "H264", .clock_rate = 90000}});
    local.codecs.insert({97, Codec{.index = 1, .name = "H264", .clock_rate = 90000}});

    remote.codecs.insert({98, Codec{.index = 2, .name = "H265", .clock_rate = 90000}});
    local.codecs.insert({98, Codec{.index = 2, .name = "H265", .clock_rate = 90000}});

    ASSERT_EQ("AV1", SelectMedia(remote, local)->codecs.at(96).name);
}

TEST_F(Av1Test, ReadWriteWithRtx) {
    const auto parsed = ParseSdp(kWebrtcAv1SdpExample);
    ASSERT_TRUE(parsed);
    ASSERT_EQ(1, parsed->medias.size());
    const auto& media = parsed->medias.front();
    ASSERT_EQ(2, media.codecs.size());
    EXPECT_EQ("AV1", media.codecs.at(45).name);
    EXPECT_EQ(90000, media.codecs.at(45).clock_rate);
    EXPECT_EQ("profile=0;level-idx=8;tier=1", media.codecs.at(45).format);
    EXPECT_EQ(RtcpFb::kNack | RtcpFb::kPli, media.codecs.at(45).rtcp_fb);
    EXPECT_EQ("apt=45", media.codecs.at(46).format);

    etl::string<4096> text;
    WriteSdp(text, *parsed);
    EXPECT_NE(etl::string_view::npos, text.find("a=rtpmap:45 AV1/90000"));
    EXPECT_NE(etl::string_view::npos, text.find("a=fmtp:45 profile=0;level-idx=8;tier=1"));

    const auto round_trip = ParseSdp(text);
    ASSERT_TRUE(round_trip);
    ASSERT_NO_FATAL_FAILURE(AssertSdp(*parsed, *round_trip));

    const auto selected = SelectMedia(media, MakeMedia());
    ASSERT_TRUE(selected);

    Sdp answer;
    answer.medias.push_back(*selected);
    WriteSdp(text, answer);

    const auto parsed_answer = ParseSdp(text);
    ASSERT_TRUE(parsed_answer);
    ASSERT_EQ(1, parsed_answer->medias.front().codecs.size());
    EXPECT_EQ("profile=0;level-idx=5;tier=0", parsed_answer->medias.front().codecs.at(45).format);
}

TEST_F(Av1Test, ReadWriteWithoutFormat) {
    Sdp source;
    source.medias.push_back(MakeMedia());

    etl::string<4096> text;
    WriteSdp(text, source);

    const auto parsed = ParseSdp(text);
    ASSERT_TRUE(parsed);
    ASSERT_NO_FATAL_FAILURE(AssertSdp(source, *parsed));
    EXPECT_TRUE(parsed->medias.front().codecs.at(96).format.empty());
}

}
