#include "apps/rtsp-client/Pipeline.h"
#include "tau/memory/SystemAllocator.h"
#include "tau/common/Random.h"
#include "tau/common/Log.h"

namespace tau::rtsp {

Pipeline::Pipeline(Allocator& allocator, Options&& options)
    : _rtp_session(
        rtp::Session::Dependencies{
            .allocator = allocator,
            .media_clock = _media_clock,
            .system_clock = _system_clock
        },
        rtp::Session::Options{
            .rate = options.clock_rate,
            .sender_ssrc = Random{}.Int<uint32_t>(),
            .base_ts = 0,
            .rtx = false,
            .send_buffer_size = 0,
            .recv_buffer_size = 4
        })
    , _h264_depacketizer(g_system_allocator)
    , _avc1_nalu_processor(h264::AvcNaluProcessor::Options{
        .type = h264::AvcNaluProcessor::Type::kAvc1,
        .sps = std::move(options.sps),
        .pps = std::move(options.pps)
    })
{
    InitPipeline();
}

void Pipeline::SetVideoCallback(VideoCallback callback) {
    _video_callback = std::move(callback);
}

void Pipeline::SetSendRtcpCallback(RtcpCallback callback) {
    _rtp_session.SetSendRtcpCallback(std::move(callback));
}

void Pipeline::RecvRtp(Buffer&& packet) {
    _rtp_session.RecvRtp(std::move(packet));
}

void Pipeline::RecvRtcp(Buffer&& packet) {
    _rtp_session.RecvRtcp(std::move(packet));
}

void Pipeline::Process() {
    _rtp_session.Process();
}

const rtp::Session::Stats& Pipeline::GetStats() const {
    return _rtp_session.GetStats();
}

void Pipeline::InitPipeline() {
    _rtp_session.SetRecvRtpCallback([this](Buffer&& rtp_packet) {
        _frame_processor.PushRtp(std::move(rtp_packet));
    });
    _frame_processor.SetCallback([this](rtp::Frame&& frame, bool losses) {
        const auto ok = !losses && _h264_depacketizer.Process(std::move(frame));
        if(!ok) {
            TAU_LOG_INFO("Drop until key-frame, frame rtp packets: " << frame.size() << (losses ? ", losses" : ""));
            _avc1_nalu_processor.DropUntilKeyFrame();
        }
    });
    _h264_depacketizer.SetCallback([this](Buffer&& nal_unit) {
        _avc1_nalu_processor.Push(std::move(nal_unit));
    });
    _avc1_nalu_processor.SetCallback([this](Buffer&& nal_unit) {
        if(_video_callback) {
            _video_callback(std::move(nal_unit));
        } else {
            _avc1_nalu_processor.DropUntilKeyFrame();
        }
    });
}

}
