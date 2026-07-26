#include "apps/rtsp-client/Client.h"
#include "tau/net/Uri.h"
#include "tau/asio/ThreadPool.h"
#include "tau/video/h264/Nalu.h"
#include "tau/video/AnnexB.h"
#include "tau/common/SystemClock.h"
#include "tau/common/Ntp.h"
#include "tau/common/File.h"
#include "tau/common/Event.h"
#include "tau/common/Log.h"

using namespace tau;
using namespace tau::rtsp;
using namespace std::chrono_literals;

int main(int argc, char** argv) {
    if(argc < 2) {
        TAU_LOG_ERROR("No RTSP stream URI (rtsp:://ip-address/path-to-stream.h264)");
        return -1;
    }
    auto uri = net::GetUriFromString(argv[1]);
    if(!uri) {
        TAU_LOG_ERROR("Malformed RTSP stream URI: " << argv[1]);
        return -1;
    }
    if(uri->protocol != net::Protocol::kRtsp) {
        TAU_LOG_ERROR("Unsupported protocol, URI: " << argv[1]);
        return -1;
    }

    ThreadPool io(1);
    SystemClock system_clock;
    std::filesystem::path output_path{std::to_string(ToNtp(system_clock.Now())) + ".h264"};

    try {
        Client client(io.GetExecutor(), Client::Options{.uri = *uri});

        client.SetVideoCallback([&](Buffer&& nal_unit) {
            const auto header = reinterpret_cast<const h264::NaluHeader*>(&nal_unit.GetView().ptr[0]);
            TAU_LOG_INFO("[H264] [avc1] nal unit type: " << (size_t)header->type << ", tp: " << etl::setprecision(3) << DurationSec(nal_unit.GetInfo().tp) << ", size: " << nal_unit.GetSize());
            auto view = nal_unit.GetView();
            //TODO: ToStringView
            WriteFile(output_path, std::string_view{reinterpret_cast<const char*>(kAnnexB.data()), kAnnexB.size()}, true);
            WriteFile(output_path, std::string_view{reinterpret_cast<const char*>(view.ptr), view.size}, true);
        });

        client.SendRequestOptions();
        client.SendRequestDescribe();
        client.SendRequestSetup();
        client.SendRequestPlay();

        Event().WaitFor(10s);

        client.SendRequestTeardown();
    } catch(const std::exception& e) {
        TAU_LOG_ERROR("Exception: " << e.what());
    }

    io.Join();
    TAU_LOG_INFO("Output path: " << output_path.string().c_str());
    TAU_LOG_INFO("Done");
    return 0;
}
