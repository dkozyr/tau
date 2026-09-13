#include "tau/rtsp/Transport.h"
#include <gtest/gtest.h>

namespace tau::rtsp {

TEST(TransportTest, RequestHeaders) {
    EXPECT_EQ("RTP/AVP/UDP;unicast;client_port=5000-5001", CreateTransportHeader(Transport::kUdp, 5000));
    EXPECT_EQ("RTP/AVP/TCP;unicast;interleaved=0-1", CreateTransportHeader(Transport::kTcp));
}

TEST(TransportTest, UdpProfiles) {
    const auto kTestValues = {
        "RTP/AVP;unicast;server_port=6000-6001",
        "RTP/AVP/UDP;unicast;server_port=6000-6001"
    };
    for(const auto& value : kTestValues) {
        const auto parameters = ParseTransport(value);
        ASSERT_TRUE(parameters);
        EXPECT_EQ(Transport::kUdp, parameters->transport);
        EXPECT_EQ(6000, parameters->server_rtp_port);
        EXPECT_FALSE(parameters->channels.has_value());
    }
}

TEST(TransportTest, TcpProfile) {
    const auto parameters = ParseTransport(" RTP/AVP/TCP ; unicast ; interleaved = 254-255 ; ssrc=12345678");
    ASSERT_TRUE(parameters);
    EXPECT_EQ(Transport::kTcp, parameters->transport);
    ASSERT_TRUE(parameters->channels.has_value());
    EXPECT_EQ(254, parameters->channels->rtp);
    EXPECT_EQ(255, parameters->channels->rtcp);
    EXPECT_FALSE(parameters->server_rtp_port.has_value());
}

TEST(TransportTest, Malformed) {
    const auto kMalformedValues = {
        "",
        "RTP/SAVP/TCP;interleaved=0-1",
        "RTP/AVP/TCP",
        "RTP/AVP;unicast",
        "RTP/AVP/TCP;interleaved=0",
        "RTP/AVP/TCP;interleaved=0-0",
        "RTP/AVP/TCP;interleaved=0-256",
        "RTP/AVP/TCP;interleaved=-1-0",
        "RTP/AVP/TCP;interleaved=0-1-2",
        "RTP/AVP/TCP;interleaved=0-a",
        "RTP/AVP/TCP;interleaved=0-18446744073709551617",
        "RTP/AVP/TCP;interleaved=0-1;interleaved=2-3",
        "RTP/AVP/TCP;multicast;interleaved=0-1",
        "RTP/AVP/TCP;server_port=6000-6001",
        "RTP/AVP;interleaved=0-1;server_port=6000-6001",
        "RTP/AVP;server_port=0-1",
        "RTP/AVP;server_port=65536-65537",
        "RTP/AVP;server_port=6000-6001;server_port=7000-7001",
        "RTP/AVP/TCP;interleaved=0-1,RTP/AVP/TCP;interleaved=2-3"
    };
    for(const auto& value : kMalformedValues) {
        EXPECT_FALSE(ParseTransport(value)) << value;
    }
}

}
