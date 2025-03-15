#include "apps/rtsp-client/Client.h"
#include "tau/asio/ThreadPool.h"
#include "tau/common/Event.h"
#include "tau/common/Log.h"

using namespace tau;
using namespace tau::rtsp;
using namespace std::chrono_literals;

int main(int /*argc*/, char** /*argv*/) {
    ThreadPool io(1);

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

        Event().WaitFor(60s);

        client.SendRequestTeardown();
    } catch(const std::exception& e) {
        LOG_ERROR << "Exception: " << e.what();
    }

    io.Join();
    LOG_INFO << "Done";
    return 0;
}
