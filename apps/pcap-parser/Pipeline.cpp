#include "apps/pcap-parser/Pipeline.h"
#include "tau/rtp/Reader.h"
#include "tau/video/h264/AnnexB.h"
#include "tau/memory/SystemAllocator.h"
#include "tau/common/Log.h"

using namespace tau;

Pipeline::Pipeline(Options&& options)
    : _pt(options.pt)
    , _output_file(options.output_file)
    , _h264_depacketizer(g_system_allocator)
    , _h265_depacketizer(g_system_allocator)
{
    BuildPipeline(options);
}

void Pipeline::Process(Buffer&& packet) {
    auto view = ToConst(packet.GetView());
    if(!rtp::Reader::Validate(view)) {
        TAU_LOG_WARNING("Skip non-rtp packet, size: " << view.size);
        return;
    }
    rtp::Reader reader(view);
    TAU_LOG_INFO("[rtp] pt: " << (size_t)reader.Pt() << ", ssrc: " << reader.Ssrc() << ", ts: " << reader.Ts() << ", sn: " << reader.Sn() << (reader.Marker() ? "*" : "") << ", payload: " << reader.Payload().size);
    if(_pt != reader.Pt()) {
        TAU_LOG_WARNING("Skip rtp packet, pt: " << (size_t)reader.Pt() << ", size: " << view.size);
        return;
    }

    _frame_processor.PushRtp(std::move(packet));
}

void Pipeline::BuildPipeline(const Options& options) {
    switch(options.type) {
        case Type::kRtpOnly:
            _frame_processor.SetCallback([this](rtp::Frame&&, bool losses) {
                if(losses) {
                    TAU_LOG_INFO("Detected packet loss");
                }
            });
            break;

        case Type::kH264:
            _frame_processor.SetCallback([this](rtp::Frame&& frame, bool losses) {
                if(losses) {
                    TAU_LOG_INFO("Detected packet loss");
                }
                if(!_h264_depacketizer.Process(std::move(frame))) {
                    TAU_LOG_WARNING("H264 depacketization failed, frame size: " << frame.size() << ", losses: " << losses);
                }
            });
            _h264_depacketizer.SetCallback([this](Buffer&& nalu) {
                WriteNaluToFile(std::move(nalu));
            });
            break;

        case Type::kH265:
            _frame_processor.SetCallback([this](rtp::Frame&& frame, bool losses) {
                if(losses) {
                    TAU_LOG_INFO("Detected packet loss");
                }
                if(!_h265_depacketizer.Process(std::move(frame))) {
                    TAU_LOG_WARNING("H265 depacketization failed, frame size: " << frame.size() << ", losses: " << losses);
                }
            });
            _h265_depacketizer.SetCallback([this](Buffer&& nalu) {
                WriteNaluToFile(std::move(nalu));
            });
            break;
    }
}

void Pipeline::WriteNaluToFile(Buffer&& nalu) {
    auto view = nalu.GetView();
    TAU_LOG_INFO("NAL unit size: " << view.size);
    WriteFile(_output_file, std::string_view{reinterpret_cast<const char*>(h264::kAnnexB.data()), h264::kAnnexB.size()}, _file_append);
    WriteFile(_output_file, std::string_view{reinterpret_cast<const char*>(view.ptr), view.size}, true);
    _file_append = true;
}
