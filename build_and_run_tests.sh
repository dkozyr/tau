#!/usr/bin/env bash
set -e

mkdir -p build
cd build
cmake -GNinja -DCMAKE_BUILD_TYPE=Debug -DGCC_SANITIZER=ON ..
ninja -j 4

./bin/tau-common-test-app
./bin/tau-asio-test-app
./bin/tau-crypto-test-app
./bin/tau-net-test-app
./bin/tau-http-test-app
./bin/tau-ws-test-app
./bin/tau-rtp-test-app
./bin/tau-rtcp-test-app
./bin/tau-rtp-packetization-test-app
./bin/tau-srtp-test-app
./bin/tau-dtls-test-app --gtest_also_run_disabled_tests=1
./bin/tau-rtp-session-test-app
./bin/tau-sdp-test-app
./bin/tau-stun-test-app
./bin/tau-mdns-test-app
./bin/tau-ice-test-app
./bin/tau-rtsp-test-app
./bin/tau-webrtc-test-app
./bin/tau-audio-test-app
./bin/tau-video-test-app
