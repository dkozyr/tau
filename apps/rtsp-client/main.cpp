#include "apps/rtsp-client/Client.h"
#include "tau/net/Uri.h"
#include "tau/asio/ThreadPool.h"
#include "tau/common/Event.h"
#include "tau/common/Log.h"

using namespace tau;
using namespace tau::rtsp;
using namespace std::chrono_literals;

int main(int argc, char** argv) {
    if(argc < 2) {
        LOG_WARNING << "No RTSP stream URI (rtsp:://ip-address/path-to-stream.h264)";
        return -1;
    }
    auto uri = net::GetUriFromString(argv[1]);
    if(!uri) {
        LOG_WARNING << "Malformed RTSP stream URI: " << argv[1];
        return -1;
    }
    if(uri->protocol != net::Protocol::kRtsp) {
        LOG_WARNING << "Unsupported protocol, URI: " << argv[1];
        return -1;
    }

    ThreadPool io(1);
    try {
        Client client(io.GetExecutor(), Client::Options{.uri = *uri});

        client.SendRequestOptions();
        client.SendRequestDescribe();
        client.SendRequestSetup();
        client.SendRequestPlay();

        Event().WaitFor(60s);

        client.SendRequestTeardown();
    } catch(const std::exception& e) {
        LOG_ERROR << "Exception: " << e.what();
    }

    io.Join();
    LOG_INFO << "Done";
    return 0;
}
