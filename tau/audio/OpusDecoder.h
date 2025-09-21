#pragma once

#include "tau/audio/DecoderInterface.h"
#include "3rdparty/opus/include/opus.h"

namespace tau::audio {

class OpusDecoder : public DecoderInterface {
public:
    static constexpr size_t kMaxFrameDurationMs = 120;
    static constexpr size_t kMaxPlcDurationMs = 400;

    struct Options {
        uint32_t sample_rate = 48000;
        uint32_t channels = 1;
        uint32_t frame_duration_ms = kDefaultFrameDurationMs;
    };

public:
    OpusDecoder(Allocator& allocator, Options&& options);
    ~OpusDecoder();

    void SetCallback(Callback callback) override { _callback = std::move(callback); }

    bool Decode(Buffer&& frame) override;
    bool Decode(const BufferViewConst& frame_view, Timepoint tp) override;
    bool DecodePlc(Timepoint tp) override;

private:
    template<typename T>
    size_t SamplesToBytes(T samples) const {
        return static_cast<size_t>(samples) * sizeof(int16_t) * _options.channels;
    }

    static uint32_t CalcFrameSamplesPerChannel(const Options& options, uint32_t frame_size);

private:
    Allocator& _allocator;
    const Options _options;
    const uint32_t _target_frame_samples_per_channel;

    ::OpusDecoder* _decoder;
    Timepoint _tp = 0;

    std::vector<uint8_t> _decode_buffer;
    const size_t _decode_buffer_samples_per_channel;

    Callback _callback;
};

}
