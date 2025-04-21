#include "tau/sdp/line/Media.h"
#include "tests/lib/Common.h"

namespace tau::sdp {

TEST(MediaReaderTest, Validate) {
    ASSERT_TRUE(MediaReader::Validate("audio 0 RTP/AVP 96"));
    ASSERT_TRUE(MediaReader::Validate("video 1 RTP/AVP 96 97 98 99 100 101"));
    ASSERT_TRUE(MediaReader::Validate("text 65535 RTP/AVP 96"));
    ASSERT_TRUE(MediaReader::Validate("application 0 RTP/AVP webrtc-datachannel"));
    ASSERT_TRUE(MediaReader::Validate("message 0 RTP/AVP 127"));
    ASSERT_TRUE(MediaReader::Validate("video 0 ANY/PROFILE/HERE 96"));

    ASSERT_FALSE(MediaReader::Validate("wrong 0 RTP/AVP 96"));
    ASSERT_FALSE(MediaReader::Validate("video 0 RTP/AVP"));
    ASSERT_FALSE(MediaReader::Validate("video 0 RTP/AVP "));
    ASSERT_FALSE(MediaReader::Validate("video 0 RTP/AVP 128"));
    ASSERT_FALSE(MediaReader::Validate("video - RTP/AVP 100"));
    ASSERT_FALSE(MediaReader::Validate("video 65536 RTP/AVP 128"));
    ASSERT_FALSE(MediaReader::Validate("video 65536 RTP/AVP 96,97"));
}

TEST(MediaReaderTest, MediaType) {
    ASSERT_EQ(MediaType::kAudio,       MediaReader::GetType("audio 0 RTP/AVP 111"));
    ASSERT_EQ(MediaType::kVideo,       MediaReader::GetType("video 0 RTP/AVP 111"));
    ASSERT_EQ(MediaType::kText,        MediaReader::GetType("text 0 RTP/AVP 111"));
    ASSERT_EQ(MediaType::kApplication, MediaReader::GetType("application 0 RTP/AVP 111"));
    ASSERT_EQ(MediaType::kMessage,     MediaReader::GetType("message 0 RTP/AVP 111"));
    ASSERT_EQ(MediaType::kUnknown,     MediaReader::GetType("wrong 0 RTP/AVP 111"));
}

TEST(MediaReaderTest, Basic) {
    std::string_view value = "video 12345 RTP/SAVPF 96 100 97 99 98";
    ASSERT_EQ(MediaType::kVideo, MediaReader::GetType(value));
    ASSERT_EQ(12345, MediaReader::GetPort(value));
    ASSERT_EQ("RTP/SAVPF", MediaReader::GetProtocol(value));
    const auto fmts = MediaReader::GetFmts(value);
    ASSERT_EQ(5, fmts.size());
    ASSERT_EQ(96, fmts[0]);
    ASSERT_EQ(100, fmts[1]);
    ASSERT_EQ(97, fmts[2]);
    ASSERT_EQ(99, fmts[3]);
    ASSERT_EQ(98, fmts[4]);
}

TEST(MediaWriterTest, Basic) {
    const auto value = MediaWriter::Write(MediaType::kVideo, 7777, "RTP/AVPF", {100, 96});
    LOG_INFO << "m=" << value;
    ASSERT_TRUE(MediaReader::Validate(value));
    ASSERT_EQ(MediaType::kVideo, MediaReader::GetType(value));
    ASSERT_EQ(7777, MediaReader::GetPort(value));
    ASSERT_EQ("RTP/AVPF", MediaReader::GetProtocol(value));
    const auto fmts = MediaReader::GetFmts(value);
    ASSERT_EQ(2, fmts.size());
    ASSERT_EQ(100, fmts[0]);
    ASSERT_EQ(96, fmts[1]);
}

}
