#pragma once

#include "tau/audio/EncoderInterface.h"
#include "3rdparty/opus/include/opus.h"

namespace tau::audio {

class OpusEncoder : public EncoderInterface {
public:
    static constexpr size_t kDtxMaxPacketSize = 2;

    struct Options {
        uint32_t sample_rate = 48000;
        uint32_t channels = 1;
        uint32_t bitrate = 64000;
        uint32_t complexity = 7;
        uint32_t fec_loss_rate = 10; // in percents
        bool dtx = true;
    };

public:
    OpusEncoder(Allocator& allocator, Options&& options);
    ~OpusEncoder();

    void SetCallback(Callback callback) override { _callback = std::move(callback); }

    bool Encode(const Buffer& frame) override;
    bool SetLossRate(uint32_t loss_rate);

private:
    Allocator& _allocator;
    const Options _options;

    ::OpusEncoder* _encoder;
    Callback _callback;
};

}
