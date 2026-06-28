#include <etl/string_view.h>

namespace tau::sdp {

inline constexpr etl::string_view kRtspSdpExample = R"(v=0
o=- 1742222222222222 1 IN IP4 192.168.0.1
s=Session streamed by "rRTSPServer"
i=ch0_0.h264
t=0 0
a=tool:LIVE555 Streaming Media v2023.01.19
a=type:broadcast
a=control:*
a=range:npt=now-
a=x-qt-text-nam:Session streamed by "rRTSPServer"
a=x-qt-text-inf:ch0_0.h264
m=video 0 RTP/AVP 96
c=IN IP4 0.0.0.0
b=AS:700
a=rtpmap:96 H264/90000
a=fmtp:96 packetization-mode=1;profile-level-id=640020;sprop-parameter-sets=Z2QAIKwsqAeAIl5ZuAgICgAAAwPoAACcQQg=,aO48sA==
a=control:track1
)";

inline constexpr etl::string_view kWebrtcAudioOnlySdpExample = R"(v=0
o=- 267107056528738969 2 IN IP4 127.0.0.1
s=-
t=0 0
a=group:BUNDLE audio
a=msid-semantic: WMS dBxfrHdjCoXIYb8pBDDDHhCGPDIG6TYDRQJ8
m=audio 9 UDP/TLS/RTP/SAVPF 111 103 104 9 0 8 106 105 13 126
c=IN IP4 0.0.0.0
a=rtcp:9 IN IP4 0.0.0.0
a=ice-ufrag:bzRv+Hl9e/MnTuO7
a=ice-pwd:YC88frVagqjvoBpOVAd+yOCH
a=fingerprint:sha-256 BE:C0:9D:93:0B:56:8C:87:48:5F:57:F7:9F:A3:D2:07:D2:8C:15:3F:DC:CE:D7:96:2B:A7:6A:DE:B8:72:F0:76
a=setup:actpass
a=mid:audio
a=extmap:1 urn:ietf:params:rtp-hdrext:ssrc-audio-level
a=extmap:3 http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time
a=sendonly
a=rtcp-mux
a=rtpmap:111 opus/48000/2
a=fmtp:111 minptime=10;useinbandfec=1
a=rtpmap:103 ISAC/16000
a=rtpmap:104 ISAC/32000
a=rtpmap:9 G722/8000
a=rtpmap:0 PCMU/8000
a=rtpmap:8 PCMA/8000
a=rtpmap:106 CN/32000
a=rtpmap:105 CN/16000
a=rtpmap:13 CN/8000
a=rtpmap:126 telephone-event/8000
a=maxptime:60
a=ssrc:655607873 cname:Wg8kdYwkkqzCZqko
a=ssrc:655607873 msid:dBxfrHdjCoXIYb8pBDDDHhCGPDIG6TYDRQJ8 920f6047-8df2-4ea5-b4f7-efff75e69688
a=ssrc:655607873 mslabel:dBxfrHdjCoXIYb8pBDDDHhCGPDIG6TYDRQJ8
a=ssrc:655607873 label:920f6047-8df2-4ea5-b4f7-efff75e69688
a=candidate:2896278100 1 udp 2122260223 192.168.1.36 63955 typ host generation 0
a=candidate:2896278100 2 udp 2122260222 192.168.1.36 59844 typ host generation 0
a=candidate:3793899172 1 tcp 1518280447 192.168.1.36 0 typ host tcptype active generation 0
a=candidate:3793899172 2 tcp 1518280446 192.168.1.36 0 typ host tcptype active generation 0
a=candidate:1521601408 1 udp 1686052607 83.49.46.37 63955 typ srflx raddr 192.168.1.36 rport 63955 generation 0
a=candidate:1521601408 2 udp 1686052606 83.49.46.37 59844 typ srflx raddr 192.168.1.36 rport 59844 generation 0
)";

inline constexpr etl::string_view kWebrtcChromeSdpExample = R"(v=0
o=- 2153471861227177332 2 IN IP4 127.0.0.1
s=-
t=0 0
a=group:BUNDLE 0 1
a=extmap-allow-mixed
a=msid-semantic: WMS 033724c4-683e-4dae-a444-dea47e2e3170
m=audio 42867 UDP/TLS/RTP/SAVPF 111 63 9 0 8 13 110 126
c=IN IP4 31.43.156.101
a=rtcp:9 IN IP4 0.0.0.0
a=candidate:1145593077 1 udp 2113937151 012e3baf-5fe2-4dfd-b538-f8fbf3141e84.local 42867 typ host generation 0 network-cost 999
a=candidate:1329961753 1 udp 1677729535 3.4.5.6 42867 typ srflx raddr 0.0.0.0 rport 0 generation 0 network-cost 999
a=ice-ufrag:DQ5F
a=ice-pwd:kj1NnowCn9vMH786My9mJzPf
a=ice-options:trickle
a=setup:actpass
a=fingerprint:sha-256 6B:E1:D7:60:D1:71:54:F5:54:95:95:09:28:E3:DF:FD:83:12:71:EA:D6:0C:D8:C2:2E:F8:CB:1C:F7:55:E1:6B
a=mid:0
a=extmap:1 urn:ietf:params:rtp-hdrext:ssrc-audio-level
a=extmap:2 http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time
a=extmap:3 http://www.ietf.org/id/draft-holmer-rmcat-transport-wide-cc-extensions-01
a=extmap:4 urn:ietf:params:rtp-hdrext:sdes:mid
a=recvonly
a=msid:033724c4-683e-4dae-a444-dea47e2e3170 5c5ba36d-f61e-49ef-aa4a-d395f2e60c60
a=rtcp-mux
a=rtcp-rsize
a=rtpmap:111 opus/48000/2
a=rtcp-fb:111 transport-cc
a=fmtp:111 minptime=10;useinbandfec=1
a=rtpmap:63 red/48000/2
a=fmtp:63 111/111
a=rtpmap:9 G722/8000
a=rtpmap:0 PCMU/8000
a=rtpmap:8 PCMA/8000
a=rtpmap:13 CN/8000
a=rtpmap:110 telephone-event/48000
a=rtpmap:126 telephone-event/8000
a=ssrc:3461839429 cname:YOXhcNpX+Cu3pUyF
a=ssrc:3461839429 msid:033724c4-683e-4dae-a444-dea47e2e3170 5c5ba36d-f61e-49ef-aa4a-d395f2e60c60
m=video 51466 UDP/TLS/RTP/SAVPF 96 97 102 103 104 105 106 107 108 109 127 125 39 40 45 46 98 99 100 101 112 113 114
c=IN IP4 31.43.156.101
a=rtcp:9 IN IP4 0.0.0.0
a=candidate:1145593077 1 udp 2113937151 012e3baf-5fe2-4dfd-b538-f8fbf3141e84.local 51466 typ host generation 0 network-cost 999
a=candidate:1329961753 1 udp 1677729535 31.43.156.101 51466 typ srflx raddr 0.0.0.0 rport 0 generation 0 network-cost 999
a=ice-ufrag:DQ5F
a=ice-pwd:kj1NnowCn9vMH786My9mJzPf
a=ice-options:trickle
a=fingerprint:sha-256 6B:E1:D7:60:D1:71:54:F5:54:95:95:09:28:E3:DF:FD:83:12:71:EA:D6:0C:D8:C2:2E:F8:CB:1C:F7:55:E1:6B
a=setup:active
a=mid:1
a=extmap:14 urn:ietf:params:rtp-hdrext:toffset
a=extmap:2 http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time
a=extmap:13 urn:3gpp:video-orientation
a=extmap:3 http://www.ietf.org/id/draft-holmer-rmcat-transport-wide-cc-extensions-01
a=extmap:5 http://www.webrtc.org/experiments/rtp-hdrext/playout-delay
a=extmap:6 http://www.webrtc.org/experiments/rtp-hdrext/video-content-type
a=extmap:7 http://www.webrtc.org/experiments/rtp-hdrext/video-timing
a=extmap:8 http://www.webrtc.org/experiments/rtp-hdrext/color-space
a=extmap:4 urn:ietf:params:rtp-hdrext:sdes:mid
a=extmap:10 urn:ietf:params:rtp-hdrext:sdes:rtp-stream-id
a=extmap:11 urn:ietf:params:rtp-hdrext:sdes:repaired-rtp-stream-id
a=sendrecv
a=msid:033724c4-683e-4dae-a444-dea47e2e3170 8c364e0c-7748-461f-9364-e4ed0bec51f4
a=rtcp-mux
a=rtcp-rsize
a=rtpmap:96 VP8/90000
a=rtcp-fb:96 goog-remb
a=rtcp-fb:96 transport-cc
a=rtcp-fb:96 ccm fir
a=rtcp-fb:96 nack
a=rtcp-fb:96 nack pli
a=rtpmap:97 rtx/90000
a=fmtp:97 apt=96
a=rtpmap:102 H264/90000
a=rtcp-fb:102 goog-remb
a=rtcp-fb:102 transport-cc
a=rtcp-fb:102 ccm fir
a=rtcp-fb:102 nack
a=rtcp-fb:102 nack pli
a=fmtp:102 level-asymmetry-allowed=1;packetization-mode=1;profile-level-id=42001f
a=rtpmap:103 rtx/90000
a=fmtp:103 apt=102
a=rtpmap:104 H264/90000
a=rtcp-fb:104 goog-remb
a=rtcp-fb:104 transport-cc
a=rtcp-fb:104 ccm fir
a=rtcp-fb:104 nack
a=rtcp-fb:104 nack pli
a=fmtp:104 level-asymmetry-allowed=1;packetization-mode=0;profile-level-id=42001f
a=rtpmap:105 rtx/90000
a=fmtp:105 apt=104
a=rtpmap:106 H264/90000
a=rtcp-fb:106 goog-remb
a=rtcp-fb:106 transport-cc
a=rtcp-fb:106 ccm fir
a=rtcp-fb:106 nack
a=rtcp-fb:106 nack pli
a=fmtp:106 level-asymmetry-allowed=1;packetization-mode=1;profile-level-id=42e01f
a=rtpmap:107 rtx/90000
a=fmtp:107 apt=106
a=rtpmap:108 H264/90000
a=rtcp-fb:108 goog-remb
a=rtcp-fb:108 transport-cc
a=rtcp-fb:108 ccm fir
a=rtcp-fb:108 nack
a=rtcp-fb:108 nack pli
a=fmtp:108 level-asymmetry-allowed=1;packetization-mode=0;profile-level-id=42e01f
a=rtpmap:109 rtx/90000
a=fmtp:109 apt=108
a=rtpmap:127 H264/90000
a=rtcp-fb:127 goog-remb
a=rtcp-fb:127 transport-cc
a=rtcp-fb:127 ccm fir
a=rtcp-fb:127 nack
a=rtcp-fb:127 nack pli
a=fmtp:127 level-asymmetry-allowed=1;packetization-mode=1;profile-level-id=4d001f
a=rtpmap:125 rtx/90000
a=fmtp:125 apt=127
a=rtpmap:39 H264/90000
a=rtcp-fb:39 goog-remb
a=rtcp-fb:39 transport-cc
a=rtcp-fb:39 ccm fir
a=rtcp-fb:39 nack
a=rtcp-fb:39 nack pli
a=fmtp:39 level-asymmetry-allowed=1;packetization-mode=0;profile-level-id=4d001f
a=rtpmap:40 rtx/90000
a=fmtp:40 apt=39
a=rtpmap:45 AV1/90000
a=rtcp-fb:45 goog-remb
a=rtcp-fb:45 transport-cc
a=rtcp-fb:45 ccm fir
a=rtcp-fb:45 nack
a=rtcp-fb:45 nack pli
a=fmtp:45 level-idx=5;profile=0;tier=0
a=rtpmap:46 rtx/90000
a=fmtp:46 apt=45
a=rtpmap:98 VP9/90000
a=rtcp-fb:98 goog-remb
a=rtcp-fb:98 transport-cc
a=rtcp-fb:98 ccm fir
a=rtcp-fb:98 nack
a=rtcp-fb:98 nack pli
a=fmtp:98 profile-id=0
a=rtpmap:99 rtx/90000
a=fmtp:99 apt=98
a=rtpmap:100 VP9/90000
a=rtcp-fb:100 goog-remb
a=rtcp-fb:100 transport-cc
a=rtcp-fb:100 ccm fir
a=rtcp-fb:100 nack
a=rtcp-fb:100 nack pli
a=fmtp:100 profile-id=2
a=rtpmap:101 rtx/90000
a=fmtp:101 apt=100
a=rtpmap:112 red/90000
a=rtpmap:113 rtx/90000
a=fmtp:113 apt=112
a=rtpmap:114 ulpfec/90000
a=ssrc-group:FID 3218536253 2426449402
a=ssrc:3218536253 cname:YOXhcNpX+Cu3pUyF
a=ssrc:3218536253 msid:033724c4-683e-4dae-a444-dea47e2e3170 8c364e0c-7748-461f-9364-e4ed0bec51f4
a=ssrc:2426449402 cname:YOXhcNpX+Cu3pUyF
a=ssrc:2426449402 msid:033724c4-683e-4dae-a444-dea47e2e3170 8c364e0c-7748-461f-9364-e4ed0bec51f4
)";

inline constexpr etl::string_view kWebrtcChromeSdpExampleWithDataChannel = R"(v=0
o=- 5192044719864126464 2 IN IP4 127.0.0.1
s=-
t=0 0
a=group:BUNDLE 0 1 2
a=extmap-allow-mixed
a=msid-semantic: WMS bbfff4ad-b85b-4e41-91cd-c973db4567c6
m=audio 9 UDP/TLS/RTP/SAVPF 111 63 9 0 8 13 110 126
c=IN IP4 0.0.0.0
a=rtcp:9 IN IP4 0.0.0.0
a=ice-ufrag:uoZi
a=ice-pwd:dDUla+y95QUYzNletnGK575w
a=ice-options:trickle
a=fingerprint:sha-256 C2:B1:05:B3:E0:ED:33:FA:A1:91:B2:6C:A3:04:18:55:B6:2A:6F:85:3C:84:0D:51:73:4E:E9:D7:2A:8F:54:61
a=setup:actpass
a=mid:0
a=extmap:1 urn:ietf:params:rtp-hdrext:ssrc-audio-level
a=extmap:2 http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time
a=extmap:3 http://www.ietf.org/id/draft-holmer-rmcat-transport-wide-cc-extensions-01
a=extmap:4 urn:ietf:params:rtp-hdrext:sdes:mid
a=sendonly
a=msid:bbfff4ad-b85b-4e41-91cd-c973db4567c6 4c10fc68-0c27-4c81-8693-8b35a2de41c6
a=rtcp-mux
a=rtcp-rsize
a=rtpmap:111 opus/48000/2
a=rtcp-fb:111 transport-cc
a=fmtp:111 minptime=10;useinbandfec=1
a=rtpmap:63 red/48000/2
a=fmtp:63 111/111
a=rtpmap:9 G722/8000
a=rtpmap:0 PCMU/8000
a=rtpmap:8 PCMA/8000
a=rtpmap:13 CN/8000
a=rtpmap:110 telephone-event/48000
a=rtpmap:126 telephone-event/8000
a=ssrc:2301497202 cname:GIyDrppR5pH7Jtim
a=ssrc:2301497202 msid:bbfff4ad-b85b-4e41-91cd-c973db4567c6 4c10fc68-0c27-4c81-8693-8b35a2de41c6
m=video 9 UDP/TLS/RTP/SAVPF 96 97 102 103 104 105 106 107 108 109 127 125 39 40 45 46 98 99 100 101 112 113 114
c=IN IP4 0.0.0.0
a=rtcp:9 IN IP4 0.0.0.0
a=ice-ufrag:uoZi
a=ice-pwd:dDUla+y95QUYzNletnGK575w
a=ice-options:trickle
a=fingerprint:sha-256 C2:B1:05:B3:E0:ED:33:FA:A1:91:B2:6C:A3:04:18:55:B6:2A:6F:85:3C:84:0D:51:73:4E:E9:D7:2A:8F:54:61
a=setup:actpass
a=mid:1
a=extmap:14 urn:ietf:params:rtp-hdrext:toffset
a=extmap:2 http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time
a=extmap:13 urn:3gpp:video-orientation
a=extmap:3 http://www.ietf.org/id/draft-holmer-rmcat-transport-wide-cc-extensions-01
a=extmap:5 http://www.webrtc.org/experiments/rtp-hdrext/playout-delay
a=extmap:6 http://www.webrtc.org/experiments/rtp-hdrext/video-content-type
a=extmap:7 http://www.webrtc.org/experiments/rtp-hdrext/video-timing
a=extmap:8 http://www.webrtc.org/experiments/rtp-hdrext/color-space
a=extmap:4 urn:ietf:params:rtp-hdrext:sdes:mid
a=extmap:10 urn:ietf:params:rtp-hdrext:sdes:rtp-stream-id
a=extmap:11 urn:ietf:params:rtp-hdrext:sdes:repaired-rtp-stream-id
a=sendonly
a=msid:bbfff4ad-b85b-4e41-91cd-c973db4567c6 e8c5d2df-7ace-4284-a322-573be9bba65a
a=rtcp-mux
a=rtcp-rsize
a=rtpmap:96 VP8/90000
a=rtcp-fb:96 goog-remb
a=rtcp-fb:96 transport-cc
a=rtcp-fb:96 ccm fir
a=rtcp-fb:96 nack
a=rtcp-fb:96 nack pli
a=rtpmap:97 rtx/90000
a=fmtp:97 apt=96
a=rtpmap:102 H264/90000
a=rtcp-fb:102 goog-remb
a=rtcp-fb:102 transport-cc
a=rtcp-fb:102 ccm fir
a=rtcp-fb:102 nack
a=rtcp-fb:102 nack pli
a=fmtp:102 level-asymmetry-allowed=1;packetization-mode=1;profile-level-id=42001f
a=rtpmap:103 rtx/90000
a=fmtp:103 apt=102
a=rtpmap:104 H264/90000
a=rtcp-fb:104 goog-remb
a=rtcp-fb:104 transport-cc
a=rtcp-fb:104 ccm fir
a=rtcp-fb:104 nack
a=rtcp-fb:104 nack pli
a=fmtp:104 level-asymmetry-allowed=1;packetization-mode=0;profile-level-id=42001f
a=rtpmap:105 rtx/90000
a=fmtp:105 apt=104
a=rtpmap:106 H264/90000
a=rtcp-fb:106 goog-remb
a=rtcp-fb:106 transport-cc
a=rtcp-fb:106 ccm fir
a=rtcp-fb:106 nack
a=rtcp-fb:106 nack pli
a=fmtp:106 level-asymmetry-allowed=1;packetization-mode=1;profile-level-id=42e01f
a=rtpmap:107 rtx/90000
a=fmtp:107 apt=106
a=rtpmap:108 H264/90000
a=rtcp-fb:108 goog-remb
a=rtcp-fb:108 transport-cc
a=rtcp-fb:108 ccm fir
a=rtcp-fb:108 nack
a=rtcp-fb:108 nack pli
a=fmtp:108 level-asymmetry-allowed=1;packetization-mode=0;profile-level-id=42e01f
a=rtpmap:109 rtx/90000
a=fmtp:109 apt=108
a=rtpmap:127 H264/90000
a=rtcp-fb:127 goog-remb
a=rtcp-fb:127 transport-cc
a=rtcp-fb:127 ccm fir
a=rtcp-fb:127 nack
a=rtcp-fb:127 nack pli
a=fmtp:127 level-asymmetry-allowed=1;packetization-mode=1;profile-level-id=4d001f
a=rtpmap:125 rtx/90000
a=fmtp:125 apt=127
a=rtpmap:39 H264/90000
a=rtcp-fb:39 goog-remb
a=rtcp-fb:39 transport-cc
a=rtcp-fb:39 ccm fir
a=rtcp-fb:39 nack
a=rtcp-fb:39 nack pli
a=fmtp:39 level-asymmetry-allowed=1;packetization-mode=0;profile-level-id=4d001f
a=rtpmap:40 rtx/90000
a=fmtp:40 apt=39
a=rtpmap:45 AV1/90000
a=rtcp-fb:45 goog-remb
a=rtcp-fb:45 transport-cc
a=rtcp-fb:45 ccm fir
a=rtcp-fb:45 nack
a=rtcp-fb:45 nack pli
a=fmtp:45 level-idx=5;profile=0;tier=0
a=rtpmap:46 rtx/90000
a=fmtp:46 apt=45
a=rtpmap:98 VP9/90000
a=rtcp-fb:98 goog-remb
a=rtcp-fb:98 transport-cc
a=rtcp-fb:98 ccm fir
a=rtcp-fb:98 nack
a=rtcp-fb:98 nack pli
a=fmtp:98 profile-id=0
a=rtpmap:99 rtx/90000
a=fmtp:99 apt=98
a=rtpmap:100 VP9/90000
a=rtcp-fb:100 goog-remb
a=rtcp-fb:100 transport-cc
a=rtcp-fb:100 ccm fir
a=rtcp-fb:100 nack
a=rtcp-fb:100 nack pli
a=fmtp:100 profile-id=2
a=rtpmap:101 rtx/90000
a=fmtp:101 apt=100
a=rtpmap:112 red/90000
a=rtpmap:113 rtx/90000
a=fmtp:113 apt=112
a=rtpmap:114 ulpfec/90000
a=ssrc-group:FID 3189205756 3045904232
a=ssrc:3189205756 cname:GIyDrppR5pH7Jtim
a=ssrc:3189205756 msid:bbfff4ad-b85b-4e41-91cd-c973db4567c6 e8c5d2df-7ace-4284-a322-573be9bba65a
a=ssrc:3045904232 cname:GIyDrppR5pH7Jtim
a=ssrc:3045904232 msid:bbfff4ad-b85b-4e41-91cd-c973db4567c6 e8c5d2df-7ace-4284-a322-573be9bba65a
m=application 9 UDP/DTLS/SCTP webrtc-datachannel
c=IN IP4 0.0.0.0
a=ice-ufrag:uoZi
a=ice-pwd:dDUla+y95QUYzNletnGK575w
a=ice-options:trickle
a=fingerprint:sha-256 C2:B1:05:B3:E0:ED:33:FA:A1:91:B2:6C:A3:04:18:55:B6:2A:6F:85:3C:84:0D:51:73:4E:E9:D7:2A:8F:54:61
a=setup:actpass
a=mid:2
a=sctp-port:5000
a=max-message-size:262144
)";

inline constexpr etl::string_view kWebrtcSafariSdpExample = R"(v=0
o=- 4417393722931175454 2 IN IP4 127.0.0.1
s=-
t=0 0
a=group:BUNDLE 0 1
a=extmap-allow-mixed
a=msid-semantic: WMS 0088aafd-7cec-4a6b-b165-f9599cdc1434
m=audio 9 UDP/TLS/RTP/SAVPF 111 63 9 0 8 13 110 126
c=IN IP4 0.0.0.0
a=rtcp:9 IN IP4 0.0.0.0
a=ice-ufrag:lGjA
a=ice-pwd:KVh9jwWy5bgHIdeSRKf4yy3O
a=ice-options:trickle
a=fingerprint:sha-256 DF:7D:1E:2D:39:BA:C5:44:DF:6F:81:D4:6D:BF:C7:0E:54:31:A3:5D:BB:BC:46:39:C0:C1:8A:69:28:AB:2F:F3
a=setup:actpass
a=mid:0
a=extmap:1 urn:ietf:params:rtp-hdrext:ssrc-audio-level
a=extmap:2 http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time
a=extmap:3 http://www.ietf.org/id/draft-holmer-rmcat-transport-wide-cc-extensions-01
a=extmap:4 urn:ietf:params:rtp-hdrext:sdes:mid
a=sendrecv
a=msid:0088aafd-7cec-4a6b-b165-f9599cdc1434 69b92c0f-45cc-458e-9b79-c3d7b9e63bf4
a=rtcp-mux
a=rtcp-rsize
a=rtpmap:111 opus/48000/2
a=rtcp-fb:111 transport-cc
a=fmtp:111 minptime=10;useinbandfec=1
a=rtpmap:63 red/48000/2
a=fmtp:63 111/111
a=rtpmap:9 G722/8000
a=rtpmap:0 PCMU/8000
a=rtpmap:8 PCMA/8000
a=rtpmap:13 CN/8000
a=rtpmap:110 telephone-event/48000
a=rtpmap:126 telephone-event/8000
a=ssrc:616985218 cname:v6qP7wQYa01mcFN8
a=ssrc:616985218 msid:0088aafd-7cec-4a6b-b165-f9599cdc1434 69b92c0f-45cc-458e-9b79-c3d7b9e63bf4
m=video 9 UDP/TLS/RTP/SAVPF 96 97 98 99 100 101 102 103 104 105 106 107 108 109 127 125 112 113 114
c=IN IP4 0.0.0.0
a=rtcp:9 IN IP4 0.0.0.0
a=ice-ufrag:lGjA
a=ice-pwd:KVh9jwWy5bgHIdeSRKf4yy3O
a=ice-options:trickle
a=fingerprint:sha-256 DF:7D:1E:2D:39:BA:C5:44:DF:6F:81:D4:6D:BF:C7:0E:54:31:A3:5D:BB:BC:46:39:C0:C1:8A:69:28:AB:2F:F3
a=setup:actpass
a=mid:1
a=extmap:14 urn:ietf:params:rtp-hdrext:toffset
a=extmap:2 http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time
a=extmap:13 urn:3gpp:video-orientation
a=extmap:3 http://www.ietf.org/id/draft-holmer-rmcat-transport-wide-cc-extensions-01
a=extmap:5 http://www.webrtc.org/experiments/rtp-hdrext/playout-delay
a=extmap:6 http://www.webrtc.org/experiments/rtp-hdrext/video-content-type
a=extmap:7 http://www.webrtc.org/experiments/rtp-hdrext/video-timing
a=extmap:8 http://www.webrtc.org/experiments/rtp-hdrext/color-space
a=extmap:4 urn:ietf:params:rtp-hdrext:sdes:mid
a=extmap:10 urn:ietf:params:rtp-hdrext:sdes:rtp-stream-id
a=extmap:11 urn:ietf:params:rtp-hdrext:sdes:repaired-rtp-stream-id
a=sendrecv
a=msid:0088aafd-7cec-4a6b-b165-f9599cdc1434 9e54933d-0d0c-423e-ad8b-e94a8b80752c
a=rtcp-mux
a=rtcp-rsize
a=rtpmap:96 H264/90000
a=rtcp-fb:96 goog-remb
a=rtcp-fb:96 transport-cc
a=rtcp-fb:96 ccm fir
a=rtcp-fb:96 nack
a=rtcp-fb:96 nack pli
a=fmtp:96 level-asymmetry-allowed=1;packetization-mode=1;profile-level-id=640c1f
a=rtpmap:97 rtx/90000
a=fmtp:97 apt=96
a=rtpmap:98 H264/90000
a=rtcp-fb:98 goog-remb
a=rtcp-fb:98 transport-cc
a=rtcp-fb:98 ccm fir
a=rtcp-fb:98 nack
a=rtcp-fb:98 nack pli
a=fmtp:98 level-asymmetry-allowed=1;packetization-mode=1;profile-level-id=42e01f
a=rtpmap:99 rtx/90000
a=fmtp:99 apt=98
a=rtpmap:100 H264/90000
a=rtcp-fb:100 goog-remb
a=rtcp-fb:100 transport-cc
a=rtcp-fb:100 ccm fir
a=rtcp-fb:100 nack
a=rtcp-fb:100 nack pli
a=fmtp:100 level-asymmetry-allowed=1;packetization-mode=0;profile-level-id=640c1f
a=rtpmap:101 rtx/90000
a=fmtp:101 apt=100
a=rtpmap:102 H264/90000
a=rtcp-fb:102 goog-remb
a=rtcp-fb:102 transport-cc
a=rtcp-fb:102 ccm fir
a=rtcp-fb:102 nack
a=rtcp-fb:102 nack pli
a=fmtp:102 level-asymmetry-allowed=1;packetization-mode=0;profile-level-id=42e01f
a=rtpmap:103 rtx/90000
a=fmtp:103 apt=102
a=rtpmap:104 H265/90000
a=rtcp-fb:104 goog-remb
a=rtcp-fb:104 transport-cc
a=rtcp-fb:104 ccm fir
a=rtcp-fb:104 nack
a=rtcp-fb:104 nack pli
a=rtpmap:105 rtx/90000
a=fmtp:105 apt=104
a=rtpmap:106 VP8/90000
a=rtcp-fb:106 goog-remb
a=rtcp-fb:106 transport-cc
a=rtcp-fb:106 ccm fir
a=rtcp-fb:106 nack
a=rtcp-fb:106 nack pli
a=rtpmap:107 rtx/90000
a=fmtp:107 apt=106
a=rtpmap:108 VP9/90000
a=rtcp-fb:108 goog-remb
a=rtcp-fb:108 transport-cc
a=rtcp-fb:108 ccm fir
a=rtcp-fb:108 nack
a=rtcp-fb:108 nack pli
a=fmtp:108 profile-id=0
a=rtpmap:109 rtx/90000
a=fmtp:109 apt=108
a=rtpmap:127 VP9/90000
a=rtcp-fb:127 goog-remb
a=rtcp-fb:127 transport-cc
a=rtcp-fb:127 ccm fir
a=rtcp-fb:127 nack
a=rtcp-fb:127 nack pli
a=fmtp:127 profile-id=2
a=rtpmap:125 rtx/90000
a=fmtp:125 apt=127
a=rtpmap:112 red/90000
a=rtpmap:113 rtx/90000
a=fmtp:113 apt=112
a=rtpmap:114 ulpfec/90000
a=ssrc-group:FID 3201680545 2693598584
a=ssrc:3201680545 cname:v6qP7wQYa01mcFN8
a=ssrc:3201680545 msid:0088aafd-7cec-4a6b-b165-f9599cdc1434 9e54933d-0d0c-423e-ad8b-e94a8b80752c
a=ssrc:2693598584 cname:v6qP7wQYa01mcFN8
a=ssrc:2693598584 msid:0088aafd-7cec-4a6b-b165-f9599cdc1434 9e54933d-0d0c-423e-ad8b-e94a8b80752c
)";

inline constexpr etl::string_view kWebrtcFirefoxSdpExample = R"(v=0
o=mozilla...THIS_IS_SDPARTA-99.0 416328760481876978 0 IN IP4 0.0.0.0
s=-
t=0 0
a=fingerprint:sha-256 AC:BC:60:D2:80:43:03:EB:F6:7C:89:11:81:FE:16:C9:A6:31:B3:33:BA:D4:28:92:FA:D3:18:54:F3:1C:3E:23
a=group:BUNDLE 0 1
a=ice-options:trickle
a=msid-semantic:WMS *
m=audio 9 UDP/TLS/RTP/SAVPF 109 9 0 8 101
c=IN IP4 0.0.0.0
a=sendonly
a=extmap:1 urn:ietf:params:rtp-hdrext:ssrc-audio-level
a=extmap:2/recvonly urn:ietf:params:rtp-hdrext:csrc-audio-level
a=extmap:3 urn:ietf:params:rtp-hdrext:sdes:mid
a=extmap-allow-mixed
a=fmtp:109 maxplaybackrate=48000;stereo=1;useinbandfec=1
a=fmtp:101 0-15
a=ice-pwd:6fa4556ab1fc0ad7adf0750f0a9d75d5
a=ice-ufrag:a42bbe2e
a=mid:0
a=msid:{1e7d7856-27f9-45af-9229-6a91cdc8f03d} {84ecf0fb-9b13-4840-b7d8-86f0fdb224db}
a=rtcp-mux
a=rtpmap:109 opus/48000/2
a=rtpmap:9 G722/8000/1
a=rtpmap:0 PCMU/8000
a=rtpmap:8 PCMA/8000
a=rtpmap:101 telephone-event/8000/1
a=setup:actpass
a=ssrc:2661432430 cname:{0081d7d9-16d1-465e-9c6f-a3f8e4832efb}
m=video 0 UDP/TLS/RTP/SAVPF 120 124 121 125 126 127 97 98 105 106 103 104 99 100 123 122 119
c=IN IP4 0.0.0.0
a=bundle-only
a=sendrecv
a=extmap:3 urn:ietf:params:rtp-hdrext:sdes:mid
a=extmap:4 http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time
a=extmap:5 urn:ietf:params:rtp-hdrext:toffset
a=extmap:6/recvonly http://www.webrtc.org/experiments/rtp-hdrext/playout-delay
a=extmap:7 http://www.ietf.org/id/draft-holmer-rmcat-transport-wide-cc-extensions-01
a=extmap-allow-mixed
a=fmtp:126 profile-level-id=42e01f;level-asymmetry-allowed=1;packetization-mode=1
a=fmtp:97 profile-level-id=42e01f;level-asymmetry-allowed=1
a=fmtp:105 profile-level-id=42001f;level-asymmetry-allowed=1;packetization-mode=1
a=fmtp:103 profile-level-id=42001f;level-asymmetry-allowed=1
a=fmtp:120 max-fs=12288;max-fr=60
a=fmtp:124 apt=120
a=fmtp:121 max-fs=12288;max-fr=60
a=fmtp:125 apt=121
a=fmtp:127 apt=126
a=fmtp:98 apt=97
a=fmtp:106 apt=105
a=fmtp:104 apt=103
a=fmtp:100 apt=99
a=fmtp:119 apt=122
a=ice-pwd:6fa4556ab1fc0ad7adf0750f0a9d75d5
a=ice-ufrag:a42bbe2e
a=mid:1
a=msid:{1e7d7856-27f9-45af-9229-6a91cdc8f03d} {92ab541a-a9d8-474f-b3ec-e9aab2317334}
a=rtcp-fb:120 nack
a=rtcp-fb:120 nack pli
a=rtcp-fb:120 ccm fir
a=rtcp-fb:120 goog-remb
a=rtcp-fb:120 transport-cc
a=rtcp-fb:121 nack
a=rtcp-fb:121 nack pli
a=rtcp-fb:121 ccm fir
a=rtcp-fb:121 goog-remb
a=rtcp-fb:121 transport-cc
a=rtcp-fb:126 nack
a=rtcp-fb:126 nack pli
a=rtcp-fb:126 ccm fir
a=rtcp-fb:126 goog-remb
a=rtcp-fb:126 transport-cc
a=rtcp-fb:97 nack
a=rtcp-fb:97 nack pli
a=rtcp-fb:97 ccm fir
a=rtcp-fb:97 goog-remb
a=rtcp-fb:97 transport-cc
a=rtcp-fb:105 nack
a=rtcp-fb:105 nack pli
a=rtcp-fb:105 ccm fir
a=rtcp-fb:105 goog-remb
a=rtcp-fb:105 transport-cc
a=rtcp-fb:103 nack
a=rtcp-fb:103 nack pli
a=rtcp-fb:103 ccm fir
a=rtcp-fb:103 goog-remb
a=rtcp-fb:103 transport-cc
a=rtcp-fb:99 nack
a=rtcp-fb:99 nack pli
a=rtcp-fb:99 ccm fir
a=rtcp-fb:99 goog-remb
a=rtcp-fb:99 transport-cc
a=rtcp-fb:123 nack
a=rtcp-fb:123 nack pli
a=rtcp-fb:123 ccm fir
a=rtcp-fb:123 goog-remb
a=rtcp-fb:123 transport-cc
a=rtcp-fb:122 nack
a=rtcp-fb:122 nack pli
a=rtcp-fb:122 ccm fir
a=rtcp-fb:122 goog-remb
a=rtcp-fb:122 transport-cc
a=rtcp-mux
a=rtcp-rsize
a=rtpmap:120 VP8/90000
a=rtpmap:124 rtx/90000
a=rtpmap:121 VP9/90000
a=rtpmap:125 rtx/90000
a=rtpmap:126 H264/90000
a=rtpmap:127 rtx/90000
a=rtpmap:97 H264/90000
a=rtpmap:98 rtx/90000
a=rtpmap:105 H264/90000
a=rtpmap:106 rtx/90000
a=rtpmap:103 H264/90000
a=rtpmap:104 rtx/90000
a=rtpmap:99 AV1/90000
a=rtpmap:100 rtx/90000
a=rtpmap:123 ulpfec/90000
a=rtpmap:122 red/90000
a=rtpmap:119 rtx/90000
a=setup:actpass
a=ssrc:1713748556 cname:{0081d7d9-16d1-465e-9c6f-a3f8e4832efb}
a=ssrc:1485109840 cname:{0081d7d9-16d1-465e-9c6f-a3f8e4832efb}
a=ssrc-group:FID 1713748556 1485109840
)";

}
