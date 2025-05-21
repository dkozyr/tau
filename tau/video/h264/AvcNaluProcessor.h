#pragma once

#include "tau/video/h264/Nalu.h"
#include "tau/memory/Buffer.h"
#include <functional>
#include <optional>

namespace tau::h264 {

// It processes input NAL units to output decodable AVC1 (same SPS/PPS for the whole stream) or AVC3 stream
class AvcNaluProcessor{
public:
    using Callback = std::function<void(Buffer&& nal_unit)>;

    enum Type {
        kAvc1 = 0,
        kAvc3 = 1,
    };

    struct Options {
        Type type = Type::kAvc1;
        std::optional<Buffer> sps = std::nullopt;
        std::optional<Buffer> pps = std::nullopt;
    };

public:
    AvcNaluProcessor(Options&& options);

    void SetCallback(Callback callback) { _callback = std::move(callback); }

    void Push(Buffer&& nal_unit);
    void DropUntilKeyFrame();

private:
    void ProcessSpsPps();

private:
    const Type _type;
    bool _drop_until_key_frame = true;
    bool _sps_pps_processed = false;
    std::optional<Buffer> _sps;
    std::optional<Buffer> _pps;
    Callback _callback;
};

}
