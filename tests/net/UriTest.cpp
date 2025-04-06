#include "tau/net/Uri.h"
#include "tests/lib/Common.h"

namespace tau::net {

TEST(UriTest, Basic) {
    auto uri = GetUriFromString("rtsp://127.0.0.1:555/stream-path.h264");
    ASSERT_TRUE(uri.has_value());
    ASSERT_EQ(Protocol::kRtsp,    uri->protocol);
    ASSERT_EQ("127.0.0.1",        uri->host);
    ASSERT_EQ(555,                uri->port);
    ASSERT_EQ("stream-path.h264", uri->path);
}

TEST(UriTest, DefaultPort_Rtsp) {
    auto uri = GetUriFromString("rtsp://domain-name.com");
    ASSERT_TRUE(uri.has_value());
    ASSERT_EQ(Protocol::kRtsp,   uri->protocol);
    ASSERT_EQ("domain-name.com", uri->host);
    ASSERT_EQ(554,               uri->port);
    ASSERT_EQ("",                uri->path);
}

TEST(UriTest, DefaultPort_Https) {
    auto uri = GetUriFromString("https://domain.online/");
    ASSERT_TRUE(uri.has_value());
    ASSERT_EQ(Protocol::kHttps, uri->protocol);
    ASSERT_EQ("domain.online",  uri->host);
    ASSERT_EQ(443,              uri->port);
    ASSERT_EQ("",               uri->path);
}

TEST(UriTest, Stun) {
    auto uri = GetUriFromString("stun:stun.l.google.com:19302");
    ASSERT_TRUE(uri.has_value());
    ASSERT_EQ(Protocol::kStun,     uri->protocol);
    ASSERT_EQ("stun.l.google.com", uri->host);
    ASSERT_EQ(19302,               uri->port);
    ASSERT_EQ(Transport::kUdp,     uri->transport.value());
    ASSERT_TRUE(uri->path.empty());
}

TEST(UriTest, TurnUdp) {
    auto uri = GetUriFromString("turn:123.44.55.66:3478?transport=udp");
    ASSERT_TRUE(uri.has_value());
    ASSERT_EQ(Protocol::kTurn,     uri->protocol);
    ASSERT_EQ("123.44.55.66",      uri->host);
    ASSERT_EQ(3478,                uri->port);
    ASSERT_EQ(Transport::kUdp,     uri->transport.value());
    ASSERT_TRUE(uri->path.empty());
}

TEST(UriTest, TurnTcp) {
    auto uri = GetUriFromString("turn:some.turn.com:3456?transport=tcp");
    ASSERT_TRUE(uri.has_value());
    ASSERT_EQ(Protocol::kTurn,     uri->protocol);
    ASSERT_EQ("some.turn.com",     uri->host);
    ASSERT_EQ(3456,                uri->port);
    ASSERT_EQ(Transport::kTcp,     uri->transport.value());
    ASSERT_TRUE(uri->path.empty());
}

TEST(UriTest, TurnWithoutPortAndTransport) {
    auto uri = GetUriFromString("turn:some-turn-server.com");
    ASSERT_TRUE(uri.has_value());
    ASSERT_EQ(Protocol::kTurn,        uri->protocol);
    ASSERT_EQ("some-turn-server.com", uri->host);
    ASSERT_EQ(3478,                   uri->port);
    ASSERT_FALSE(uri->transport.has_value());
    ASSERT_TRUE(uri->path.empty());
}

TEST(UriTest, Valid) {
    ASSERT_TRUE(GetUriFromString("rtsp://hello:0/world//////").has_value());
    ASSERT_TRUE(GetUriFromString("http://h:65535///~!@#$%^&*()_+").has_value());
    ASSERT_TRUE(GetUriFromString("stun:www").has_value());
}

TEST(UriTest, NotSupportedProtocol) {
    ASSERT_FALSE(GetUriFromString("stuns:stun-server.com").has_value());
    ASSERT_FALSE(GetUriFromString("turns:server.com").has_value());
}

TEST(UriTest, MalformedProtocol) {
    ASSERT_FALSE(GetUriFromString("rtsp::/127.0.0.1").has_value());
    ASSERT_FALSE(GetUriFromString("rtsp:/127.0.0.1").has_value());
    ASSERT_FALSE(GetUriFromString("rtsp+//127.0.0.1").has_value());
    ASSERT_FALSE(GetUriFromString("rtsp+127.0.0.1").has_value());
    ASSERT_FALSE(GetUriFromString(" rtsp://127.0.0.1").has_value());
    ASSERT_FALSE(GetUriFromString("https ://127.0.0.1").has_value());
    ASSERT_FALSE(GetUriFromString("http://127.0.0.1/http://domain.com").has_value());
}

TEST(UriTest, MalformedHost) {
    ASSERT_FALSE(GetUriFromString("rtsp:///stream").has_value());
    ASSERT_FALSE(GetUriFromString("rtsp://127 0.0.1/stream").has_value());
    ASSERT_FALSE(GetUriFromString("rtsp://domain%name.com/stream").has_value());
    ASSERT_FALSE(GetUriFromString("rtsp://127 0.0.1:554:555/stream").has_value());
}

TEST(UriTest, WrongPort) {
    ASSERT_FALSE(GetUriFromString("rtsp://127.0.0.1:65536").has_value());
    ASSERT_FALSE(GetUriFromString("rtsp://127.0.0.1:/stream").has_value());
}

}
