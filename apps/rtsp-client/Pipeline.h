#pragma once

#include "apps/rtsp-client/PipelineOptions.h"
#include "tau/rtp-session/Session.h"
#include "tau/rtp-session/FrameProcessor.h"
#include "tau/rtp-packetization/H264Depacketizer.h"
#include "tau/video/h264/AvcNaluProcessor.h"
#include "tau/common/SystemClock.h"
#include "tau/common/SteadyClock.h"

namespace tau::rtsp {

class Pipeline {
public:
    using Options = PipelineOptions;
    using VideoCallback = std::function<void(Buffer&& nal_unit)>;
    using RtcpCallback = std::function<void(Buffer&& packet)>;

public:
    Pipeline(Allocator& allocator, Options&& options);

    void SetVideoCallback(VideoCallback callback);

    // Set before receiving packets or calling Process
    void SetSendRtcpCallback(RtcpCallback callback);
    void RecvRtp(Buffer&& packet);
    void RecvRtcp(Buffer&& packet);
    void Process();

    const rtp::Session::Stats& GetStats() const;

private:
    void InitPipeline();

private:
    SteadyClock _media_clock;
    SystemClock _system_clock;

    rtp::Session _rtp_session;
    rtp::session::FrameProcessor _frame_processor;
    rtp::H264Depacketizer _h264_depacketizer;
    h264::AvcNaluProcessor _avc1_nalu_processor;

    VideoCallback _video_callback;
};

}
