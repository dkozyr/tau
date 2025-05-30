#!/usr/bin/env bash
set -e

mkdir -p build
cd build
cmake -GNinja -DCMAKE_BUILD_TYPE=Debug -DGCC_SANITIZER=ON ..
ninja -j 4

./bin/common-test-app
./bin/asio-test-app
./bin/crypto-test-app
./bin/net-test-app
./bin/http-test-app
./bin/ws-test-app
./bin/rtp-test-app
./bin/rtcp-test-app
./bin/rtp-packetization-test-app
./bin/srtp-test-app
./bin/dtls-test-app --gtest_also_run_disabled_tests=1
./bin/rtp-session-test-app
./bin/sdp-test-app
./bin/stun-test-app
./bin/mdns-test-app
./bin/ice-test-app
./bin/rtsp-test-app
./bin/webrtc-test-app
./bin/video-test-app
