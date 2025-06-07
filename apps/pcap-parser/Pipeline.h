#pragma once

#include "tau/rtp-packetization/H265Depacketizer.h"
#include "tau/rtp-packetization/H264Depacketizer.h"
#include "tau/rtp-session/FrameProcessor.h"
#include "tau/memory/Buffer.h"
#include "tau/common/File.h"

class Pipeline {
public:
    enum Type : size_t {
        kRtpOnly = 0,
        kH264    = 1,
        kH265    = 2,
    };

    struct Options {
        Type type;
        uint8_t pt;
        std::filesystem::path output_file;
    };

public:
    Pipeline(Options&& options);

    void Process(tau::Buffer&& packet);

private:
    void BuildPipeline(const Options& options);
    void WriteNaluToFile(tau::Buffer&& nalu);

private:
    const uint8_t _pt;
    bool _file_append = false;
    std::filesystem::path _output_file;
    tau::rtp::H264Depacketizer _h264_depacketizer;
    tau::rtp::H265Depacketizer _h265_depacketizer;
    tau::rtp::session::FrameProcessor _frame_processor;
};
