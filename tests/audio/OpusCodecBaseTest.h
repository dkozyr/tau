#include "tau/audio/OpusEncoder.h"
#include "tau/audio/OpusDecoder.h"
#include "tests/audio/CodecBaseTest.h"

namespace tau::audio {

class OpusCodecBaseTest : public CodecBaseTest {
public:
    OpusCodecBaseTest() : CodecBaseTest() {}

protected:
    void Init() {
        InitEncoder();
        InitDecoder(_sample_rate);
    }

    void InitEncoder() {
        InitParams(_options.sample_rate, _options.channels);

        _encoder = std::make_unique<OpusEncoder>(_rtp_allocator, OpusEncoder::Options{_options});
        _encoder->SetCallback([this](Buffer&& encoded_frame) {
            // TAU_LOG_INFO("Encoded frame size: " << encoded_frame.GetView().size << ", tp: " << (int)DurationMs(encoded_frame.GetInfo().tp));
            _encoded_frames.push_back(encoded_frame.MakeCopy());

            EXPECT_TRUE(_decoder->Decode(std::move(encoded_frame)));
        });
    }

    void InitDecoder(uint32_t sample_rate, uint32_t frame_duration_ms = OpusDecoder::kDefaultFrameDurationMs) {
        _decoder = std::make_unique<OpusDecoder>(
            _frame_allocator,
            OpusDecoder::Options{
                .sample_rate = sample_rate,
                .channels = _channels,
                .frame_duration_ms = frame_duration_ms
            });
        _decoder->SetCallback([this](Buffer&& decoded_frame) {
            // TAU_LOG_INFO("Decoded frame size: " << decoded_frame.GetView().size << ", tp: " << (int)DurationMs(decoded_frame.GetInfo().tp));
            _decoded_frames.push_back(std::move(decoded_frame));
        });
    }

    void InitDecodingWithPlc(bool test_plc_result = true) {
        _encoder->SetCallback([this, test_plc_result](Buffer&& encoded_frame) {
            // TAU_LOG_INFO("Encoded frame size: " << encoded_frame.GetView().size << ", tp: " << (int)DurationMs(encoded_frame.GetInfo().tp));
            _encoded_frames.push_back(encoded_frame.MakeCopy());

            auto plc_result = _decoder->DecodePlc(encoded_frame.GetInfo().tp);
            if(test_plc_result) {
                EXPECT_TRUE(plc_result);
            }
            EXPECT_TRUE(_decoder->Decode(std::move(encoded_frame)));
        });
    }

protected:
    OpusEncoder::Options _options = {};
    std::unique_ptr<OpusEncoder> _encoder;
    std::unique_ptr<OpusDecoder> _decoder;
};

}
