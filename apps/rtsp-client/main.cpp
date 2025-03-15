#include "apps/rtsp-client/Client.h"
#include "apps/rtsp-client/Utils.h"
#include "tau/rtp-session/Session.h"
#include "tau/rtp-session/FrameProcessor.h"
#include "tau/rtp-packetization/H264Depacketizer.h"
#include "tau/video/h264/Avc1NaluProcessor.h"
#include "tau/rtsp/Request.h"
#include "tau/rtsp/RequestWriter.h"
#include "tau/rtp/Reader.h"
#include "tau/net/UdpSocketsPair.h"
#include "tau/memory/PoolAllocator.h"
#include "tau/memory/SystemAllocator.h"
#include "tau/asio/ThreadPool.h"
#include "tau/common/File.h"
#include "tau/common/Ntp.h"
#include "tau/common/Event.h"
#include "tau/common/Random.h"

using namespace tau;
using namespace tau::rtsp;
using namespace std::chrono_literals;

int main(int /*argc*/, char** /*argv*/) {
    ThreadPool io(1); // +50 Kb

    try {
        Client client(io.GetExecutor(),
            Client::Options{
                .uri = "rtsp://192.168.0.167/ch0_0.h264",
                .host = "192.168.0.167"
            });

        client.SendRequestOptions();
        client.SendRequestDescribe();
        client.SendRequestSetup();
        client.SendRequestPlay();

        Event().WaitFor(10s);

        client.SendRequestTeardown();
    } catch(const std::exception& e) {
        LOG_ERROR << "Exception: " << e.what();
    }

    io.Join();
    LOG_INFO << "Done";
    return 0;
}
