#include "tau/rtsp/StreamReader.h"
#include "tau/rtsp/ResponseReader.h"
#include <gtest/gtest.h>
#include <vector>
#include <string>

namespace tau::rtsp {

using namespace std::string_literals;

class StreamReaderTest : public ::testing::Test {
public:
    const std::string kResponse = "RTSP/1.0 200 OK\r\nCSeq: 1\r\nContent-Length: 5\r\n\r\n$\r\n\0x"s;

public:
    static std::string Frame(uint8_t channel, const std::string& payload) {
        std::string frame{'$', static_cast<char>(channel), static_cast<char>(payload.size() / 256), static_cast<char>(payload.size() % 256)};
        return frame + payload;
    }
};


TEST_F(StreamReaderTest, EverySplitBoundary) {
    const auto input = kResponse + Frame(254, std::string{"\0$\r\n", 4}) + "RTSP/1.0 200 OK\r\nCSeq: 2\r\n\r\n";
    for(size_t i = 0; i <= input.size(); ++i) {
        std::vector<StreamReader::Message> messages;
        const auto callback = [&](StreamReader::Message&& message) {
            messages.push_back(std::move(message));
        };

        StreamReader reader;
        ASSERT_TRUE(reader.Push({input.data(), i}, callback));
        ASSERT_TRUE(reader.Push({input.data() + i, input.size() - i}, callback));
        ASSERT_TRUE(reader.Finish());
        ASSERT_EQ(3, messages.size());
        EXPECT_FALSE(messages[0].channel);
        EXPECT_EQ(kResponse, (std::string{messages[0].data.data(), messages[0].data.size()}));
        EXPECT_EQ(254, messages[1].channel);
        EXPECT_EQ((etl::string_view{"\0$\r\n", 4}), (etl::string_view{messages[1].data.data(), messages[1].data.size()}));
        EXPECT_FALSE(messages[2].channel);
    }
}

TEST_F(StreamReaderTest, ByteAtATimeAndRetainedMessage) {
    std::optional<StreamReader::Message> saved;
    const auto callback = [&](StreamReader::Message&& message) {
        saved.emplace(std::move(message));
    };

    StreamReader reader;
    for(const auto character : kResponse) {
        ASSERT_TRUE(reader.Push({&character, 1}, callback));
    }
    ASSERT_TRUE(saved);

    const auto response = ResponseReader::Read({saved->data.data(), saved->data.size()});
    ASSERT_TRUE(response);

    const auto copy = *response;
    reader.Reset();
    ASSERT_TRUE(reader.Push("RTSP/1.0 200 OK\r\nCSeq: 99\r\n\r\n", [](StreamReader::Message&&) {}));
    EXPECT_EQ("1", GetHeaderValue(HeaderName::kCSeq, copy.headers));
    EXPECT_EQ((etl::string_view{"$\r\n\0x", 5}), copy.body);
    EXPECT_EQ("OK", copy.reason_phrase);
}

TEST_F(StreamReaderTest, LargePackets) {
    StreamReader reader;
    const std::string payload(65535, '$');
    const auto input = Frame(255, payload);
    size_t count = 0;
    ASSERT_TRUE(reader.Push({input.data(), input.size()}, [&](StreamReader::Message&& message) {
        EXPECT_EQ(255, message.channel);
        EXPECT_EQ(payload, (std::string{message.data.data(), message.data.size()}));
        ++count;
    }));
    EXPECT_EQ(1, count);
    EXPECT_TRUE(reader.Finish());
}

TEST_F(StreamReaderTest, InvalidLengthsAndHeaders) {
    const auto kMalformedHeaders = {
        "Content-Length: -1",
        "Content-Length: +1",
        "Content-Length: x",
        "Content-Length:",
        "Content-Length: 184467440737095516160",
        "Content-Length: 1048577",
        "Content-Length: 0\r\ncontent-length: 0",
        "Invalid",
        ": value"
    };
    for(const auto& headers : kMalformedHeaders) {
        StreamReader reader;
        const auto input = std::string{"RTSP/1.0 200 OK\r\n"} + headers + "\r\n\r\n";
        const auto callback = [](StreamReader::Message&&) {
            FAIL() << "Unexpected message";
        };
        ASSERT_FALSE(reader.Push({input.data(), input.size()}, callback)) << headers;
        ASSERT_FALSE(reader.Finish());
        ASSERT_FALSE(reader.Push("", callback));
        reader.Reset();
        ASSERT_TRUE(reader.Finish());
    }
}

TEST_F(StreamReaderTest, LimitsAndTruncatedInput) {
    const auto callback = [](StreamReader::Message&&) {};

    StreamReader reader({.max_header_size = 32, .max_packet_size = 3});
    EXPECT_FALSE(reader.Push("RTSP/1.0 200 OK\r\nLong: 12345678901234567890", callback));
    reader.Reset();

    auto packet = Frame(0, "four");
    EXPECT_FALSE(reader.Push({packet.data(), packet.size()}, callback));

    reader.Reset();
    packet = Frame(0, "");
    EXPECT_FALSE(reader.Push({packet.data(), packet.size()}, callback));
    for(const auto partial : {"$", "$\1", "RTSP/1.0 200 OK\r\n", "RTSP/1.0 200 OK\r\nContent-Length: 2\r\n\r\nx"}) {
        StreamReader incomplete;
        ASSERT_TRUE(incomplete.Push(partial, callback));
        EXPECT_FALSE(incomplete.Finish());
    }
}

TEST_F(StreamReaderTest, ExactLimitsAndCaseInsensitiveLength) {
    size_t count = 0;
    const auto callback = [&](StreamReader::Message&&) {
        ++count;
    };

    const std::string headers = "RTSP/1.0 200 OK\r\ncOnTeNt-LeNgTh: 3\r\n\r\n";
    StreamReader reader({.max_header_size = headers.size(), .max_packet_size = 3});
    const auto input = headers + "abc" + Frame(0, "abc");
    EXPECT_TRUE(reader.Push({input.data(), input.size()}, callback));
    EXPECT_EQ(2, count);
    EXPECT_TRUE(reader.Finish());
}

TEST_F(StreamReaderTest, FixedCapacity) {
    const std::string headers = "RTSP/1.0 200 OK\r\nContent-Length: 65494\r\n\r\n";
    ASSERT_EQ(StreamReader::kCapacity, headers.size() + 65494);
    const auto input = headers + std::string(65494, 'x');
    StreamReader reader;
    size_t count = 0;
    ASSERT_TRUE(reader.Push({input.data(), input.size()}, [&](StreamReader::Message&& message) {
        EXPECT_EQ(StreamReader::kCapacity, message.data.size());
        ++count;
    }));
    EXPECT_EQ(1, count);
    EXPECT_TRUE(reader.Finish());
    EXPECT_FALSE(reader.Push("RTSP/1.0 200 OK\r\nContent-Length: 65495\r\n\r\n", [](StreamReader::Message&&) {}));
}

TEST_F(StreamReaderTest, TruncatedPacketAfterHeader) {
    StreamReader reader;
    const auto input = Frame(0, "x");
    EXPECT_TRUE(reader.Push({input.data(), 4}, [](StreamReader::Message&&) {}));
    EXPECT_FALSE(reader.Finish());
}

TEST_F(StreamReaderTest, HeaderValuesWithColons) {
    size_t count = 0;
    const auto callback = [&](StreamReader::Message&&) {
        ++count;
    };
    StreamReader reader;
    EXPECT_TRUE(reader.Push("RTSP/1.0 200 OK\r\nContent-Base: rtsp://127.0.0.1:554/video\r\nContent-Length: 3\r\n\r\nabc", callback));
    EXPECT_EQ(1, count);
}

}
