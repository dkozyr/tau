#include <string_view>

namespace tau::sdp {

inline constexpr std::string_view kRtspSdpExample = R"(v=0
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

inline constexpr std::string_view kWebrtcAudioOnlySdpExample = R"(v=0
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

inline constexpr std::string_view kWebrtcChromeSdpExample = R"(v=0
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

inline constexpr std::string_view kWebrtcChromeSdpExampleWithDataChannel = R"(v=0
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

}
