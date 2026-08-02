#include "apps/rtsp-client/Client.h"
#include "apps/rtsp-to-webrtc-client/Device.h"
#include "tau/srtp/Common.h"
#include "tau/net/Uri.h"
#include "tau/memory/PoolAllocator.h"
#include "tau/asio/ThreadPool.h"
#include "tau/video/h264/Nalu.h"
#include "tau/video/AnnexB.h"
#include "tau/common/CrashSignalHandler.h"
#include "tau/common/SystemClock.h"
#include "tau/common/Ntp.h"
#include "tau/common/File.h"
#include "tau/common/Event.h"
#include "tau/common/Log.h"

using namespace tau;
using namespace tau::rtsp;
using namespace std::chrono_literals;

const etl::string_view kLocalHost = "127.0.0.1";
const uint16_t kSignallingPort = 12345;

const char kCaCertPath[] = PROJECT_SOURCE_DIR "/data/keys/ca.crt";
const char kCaKeyPath[]  = PROJECT_SOURCE_DIR "/data/keys/ca.key";

std::array<uint8_t, 32 * 1024 * 1024> g_allocated_memory;
PoolAllocator g_udp_allocator(g_allocated_memory.data(), g_allocated_memory.size(), 1200);

std::optional<Client> g_rtsp_client;
std::optional<signalling::Device> g_device;

// bash build_and_run_tests.sh
// ./build/bin/tau-signalling-server-test-app --gtest_filter=*MANUAL* --gtest_also_run_disabled_tests=1

void GracefulShutdownHandler(int) {
    if(g_device) {
        g_device.reset();
    }
    if(g_rtsp_client) {
        g_rtsp_client->SendRequestTeardown();
        g_rtsp_client.reset();
    }
    TAU_LOG_INFO("OK");
    std::exit(1);
}

int main(int argc, char** argv) {
    CrashSignalHandler crash_signal_handler;
    signal(SIGINT, GracefulShutdownHandler);
    srtp::Init();

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

    ThreadPool io(4); //TODO: fix it
    SteadyClock steady_clock;
    SystemClock system_clock;
    std::filesystem::path output_path{std::to_string(ToNtp(system_clock.Now())) + ".h264"};

    try {
        g_rtsp_client.emplace(io.GetExecutor(), Client::Options{.uri = *uri});

        g_rtsp_client->SetVideoCallback([&](Buffer&& nal_unit) {
            const auto header = reinterpret_cast<const h264::NaluHeader*>(&nal_unit.GetView().ptr[0]);
            if(header->type == h264::kIdr) {
                TAU_LOG_INFO("[H264] [avc1] nal unit type: " << (size_t)header->type << ", tp: " << etl::setprecision(3) << DurationSec(nal_unit.GetInfo().tp) << ", size: " << nal_unit.GetSize());
            }
            auto view = nal_unit.GetView();
            //TODO: ToStringView
            WriteFile(output_path, std::string_view{reinterpret_cast<const char*>(kAnnexB.data()), kAnnexB.size()}, true);
            WriteFile(output_path, std::string_view{reinterpret_cast<const char*>(view.ptr), view.size}, true);

            g_device->SendFrame(std::move(nal_unit));
        });

        g_rtsp_client->SendRequestOptions();
        g_rtsp_client->SendRequestDescribe();
        g_rtsp_client->SendRequestSetup();
        g_rtsp_client->SendRequestPlay();

        //TODO: get video options from Client
        crypto::Certificate ca(crypto::Certificate::Options{
            .cert = etl::string_view{kCaCertPath},
            .key  = etl::string_view{kCaKeyPath}
        });
        crypto::Certificate client_certificate(
            crypto::Certificate::OptionsSelfSigned{.ca = ca}
        );
        auto client_ssl_ctx = CreateSslContextPtr(
            client_certificate.GetCertificateBuffer(),
            client_certificate.GetPrivateKeyBuffer()
        );

        g_device.emplace(
            signalling::Device::Dependencies{
                .executor = io.GetExecutor(),
                .clock = steady_clock,
                .udp_allocator = g_udp_allocator
            },
            signalling::Device::Options{
                .device_id = 0x42424242,
                .ws_options = ws::Client::Options{kLocalHost, kSignallingPort, "/device", *client_ssl_ctx}
            }
        );

        std::optional<signalling::StreamId> device_stream_id;
        Event on_stream_id_ready;
        g_device->SetStreamIdCallback([&](signalling::StreamId stream_id) {
            device_stream_id = stream_id;
            on_stream_id_ready.Set();
        });
        Event on_device_connected;
        g_device->SetStateChangeCallback([&](webrtc::State state) {
            if(state == webrtc::State::kConnected) {
                on_device_connected.Set();
                TAU_LOG_INFO("Device connected.....");
            }
        });
        if(!g_device->Start()) {
            TAU_LOG_ERROR("Device start failed");
        }
        if(on_stream_id_ready.WaitFor(1s)) {
            TAU_LOG_INFO("Stream id: " << device_stream_id.value_or(0));
        } else {
            TAU_LOG_ERROR("Device is not ready");
        }

        Event().WaitFor(600s);

        g_rtsp_client->SendRequestTeardown();
        g_rtsp_client.reset();
    } catch(const std::exception& e) {
        TAU_LOG_ERROR("Exception: " << e.what());
    }

    io.Join();
    TAU_LOG_INFO("Output path: " << output_path.string().c_str());
    TAU_LOG_INFO("Done");
    return 0;
}
