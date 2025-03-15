#pragma once

#include "tau/video/h264/Nalu.h"
#include "tau/memory/Buffer.h"
#include <functional>
#include <optional>

namespace tau::h264 {

// It processes input NAL units to output decodable AVC1 stream (same SPS/PPS for the whole stream)
class Avc1NaluProcessor{
public:
    using Callback = std::function<void(Buffer&& nal_unit)>;

    struct Options {
        std::optional<Buffer> sps = std::nullopt;
        std::optional<Buffer> pps = std::nullopt;
    };

public:
    Avc1NaluProcessor(Options&& options);

    void SetCallback(Callback callback) { _callback = std::move(callback); }

    void Push(Buffer&& nal_unit);
    void DropUntilKeyFrame();

private:
    void ProcessSpsPps();

private:
    bool _drop_until_key_frame = true;
    bool _sps_pps_processed = false;
    std::optional<Buffer> _sps;
    std::optional<Buffer> _pps;
    Callback _callback;
};

}
