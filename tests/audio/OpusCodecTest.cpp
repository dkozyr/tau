#include "tests/audio/OpusCodecBaseTest.h"
#include "tests/audio/OpusCodecTestParams.h"

namespace tau::audio {

class OpusCodecTest
    : public OpusCodecBaseTest
    , public ::testing::TestWithParam<OpusCodecTestParams> {
public:
    OpusCodecTest() {
        _options.sample_rate = GetParam().sample_rate;
        _options.channels = GetParam().channels;
        Init();
    }
};

TEST_P(OpusCodecTest, DifferentFrameSizes) {
    for(auto frame_ms : {5, 10, 20, 60, 100, 120}) {
        auto samples_per_channel = CalcFrameSamples(frame_ms, _options.sample_rate);
        auto samples_offset = g_random.Int<size_t>(0, _pcm_samples_count - samples_per_channel * _options.channels);
        ASSERT_TRUE(_encoder->Encode(CreateFrame(samples_offset, samples_per_channel)));
        ASSERT_EQ(SamplesOffsetToTimepoint(samples_offset), _encoded_frames.back().GetInfo().tp);

        ASSERT_FALSE(_encoder->Encode(CreateFrame(0, samples_per_channel - 1)));
        ASSERT_FALSE(_encoder->Encode(CreateFrame(0, samples_per_channel + 1)));
    }
}

TEST_P(OpusCodecTest, DecodeToDefaultSampleRate) {
    InitDecoder(48000);

    auto samples_offset = 0;
    auto samples_per_channel = CalcFrameSamples(20, _options.sample_rate);
    for(size_t i = 0; i < 10; ++i) {
        ASSERT_TRUE(_encoder->Encode(CreateFrame(samples_offset, samples_per_channel)));
        ASSERT_EQ(i + 1, _decoded_frames.size());
        ASSERT_EQ(SamplesOffsetToTimepoint(samples_offset), _decoded_frames.back().GetInfo().tp);
        samples_offset += samples_per_channel * _options.channels;
    }
}

TEST_P(OpusCodecTest, DecodeToCustomFrameDuration) {
    constexpr auto kSampleRate = 48000; 
    constexpr auto kFrameSizeMs = 5; 
    InitDecoder(kSampleRate, kFrameSizeMs);

    auto samples_offset = 0;
    auto samples_per_channel = CalcFrameSamples(20, _options.sample_rate);
    for(size_t i = 0; i < 10; ++i) {
        ASSERT_TRUE(_encoder->Encode(CreateFrame(samples_offset, samples_per_channel)));
        ASSERT_EQ((i + 1) * 4, _decoded_frames.size());
        ASSERT_EQ(SamplesOffsetToTimepoint(samples_offset) + 15 * kMs, _decoded_frames.back().GetInfo().tp);
        samples_offset += samples_per_channel * _options.channels;
    }

    Timepoint expected_tp = 0;
    auto expected_frame_samples_per_channel = kSampleRate * kFrameSizeMs / 1000;
    auto expected_frame_size = expected_frame_samples_per_channel * sizeof(int16_t) * _options.channels;
    for(const auto& frame : _decoded_frames) {
        ASSERT_EQ(expected_frame_size, frame.GetView().size);
        ASSERT_EQ(expected_tp, frame.GetInfo().tp);
        expected_tp += 5 * kMs;
    }
}

TEST_P(OpusCodecTest, Bitrate) {
    constexpr auto kBitrateMarginBps = 2000;
    {
        _options.bitrate = 8000;
        InitEncoder();

        size_t bytes = 0;
        auto samples_offset = 0;
        auto samples_per_channel = CalcFrameSamples(20, _options.sample_rate);
        for(size_t i = 0; i < 2 * 50; ++i) {
            ASSERT_TRUE(_encoder->Encode(CreateFrame(samples_offset, samples_per_channel)));
            bytes += _encoded_frames.back().GetView().size;
            samples_offset += samples_per_channel * _options.channels;
        }
        auto bitrate = bytes * 8 / 2;
        ASSERT_GT(_options.bitrate + kBitrateMarginBps, bitrate);
        ASSERT_LT(_options.bitrate / 2, bitrate);
    }
    {
        _options.bitrate = 64000;
        InitEncoder();

        size_t bytes = 0;
        auto samples_offset = 0;
        auto samples_per_channel = CalcFrameSamples(20, _options.sample_rate);
        for(size_t i = 0; i < 100; ++i) {
            ASSERT_TRUE(_encoder->Encode(CreateFrame(samples_offset, samples_per_channel)));
            bytes += _encoded_frames.back().GetView().size;
            samples_offset += samples_per_channel * _options.channels;
        }
        auto bitrate = bytes * 8 / 2;
        ASSERT_GT(_options.bitrate + kBitrateMarginBps, bitrate);
        ASSERT_LT(_options.bitrate / 2, bitrate);
    }
}

TEST_P(OpusCodecTest, PacketLoss) {
    InitDecodingWithPlc();

    auto samples_offset = 0;
    auto samples_per_channel = CalcFrameSamples(20, _options.sample_rate);
    ASSERT_TRUE(_encoder->Encode(CreateFrame(samples_offset, samples_per_channel)));
    ASSERT_EQ(1, _decoded_frames.size());
    ASSERT_EQ(SamplesOffsetToTimepoint(samples_offset), _decoded_frames.back().GetInfo().tp);
    samples_offset += samples_per_channel * _options.channels;

    constexpr auto kLostFrames = 4;
    samples_offset += kLostFrames * samples_per_channel * _options.channels;
    ASSERT_TRUE(_encoder->Encode(CreateFrame(samples_offset, samples_per_channel)));
    ASSERT_EQ(1 + 4 + 1, _decoded_frames.size());
    ASSERT_EQ(SamplesOffsetToTimepoint(samples_offset), _decoded_frames.back().GetInfo().tp);
}

TEST_P(OpusCodecTest, LostHalfFrame) {
    InitDecodingWithPlc();

    auto samples_offset = 0;
    auto samples_per_channel = CalcFrameSamples(20, _options.sample_rate);
    ASSERT_TRUE(_encoder->Encode(CreateFrame(samples_offset, samples_per_channel)));
    ASSERT_EQ(1, _decoded_frames.size());
    ASSERT_EQ(SamplesOffsetToTimepoint(samples_offset), _decoded_frames.back().GetInfo().tp);

    samples_offset += samples_per_channel* _options.channels * 3 / 2;
    ASSERT_TRUE(_encoder->Encode(CreateFrame(samples_offset, samples_per_channel)));
    ASSERT_EQ(1 + 1 + 1, _decoded_frames.size());
    ASSERT_EQ(SamplesOffsetToTimepoint(samples_offset), _decoded_frames.back().GetInfo().tp);
}

TEST_P(OpusCodecTest, IgnoreBigGap) {
    InitDecodingWithPlc(false);

    auto samples_offset = 0;
    auto samples_per_channel = CalcFrameSamples(20, _options.sample_rate);
    ASSERT_TRUE(_encoder->Encode(CreateFrame(samples_offset, samples_per_channel)));
    ASSERT_EQ(1, _decoded_frames.size());
    ASSERT_EQ(SamplesOffsetToTimepoint(samples_offset), _decoded_frames.back().GetInfo().tp);

    samples_offset += _options.sample_rate;
    ASSERT_TRUE(_encoder->Encode(CreateFrame(samples_offset, samples_per_channel)));
    ASSERT_EQ(1 + 1, _decoded_frames.size());
    ASSERT_EQ(SamplesOffsetToTimepoint(samples_offset), _decoded_frames.back().GetInfo().tp);
}

TEST_P(OpusCodecTest, DtxOnSilence) {
    auto samples_per_channel = CalcFrameSamples(20, _options.sample_rate);
    auto samples_offset = g_random.Int<size_t>();
    for(size_t i = 0; i < 10; ++i) {
        ASSERT_TRUE(_encoder->Encode(CreateSilenceFrame(samples_offset, samples_per_channel)));
        ASSERT_LT(OpusEncoder::kDtxMaxPacketSize, _encoded_frames.back().GetView().size);
        samples_offset += samples_per_channel * _options.channels;
    }

    ASSERT_TRUE(_encoder->Encode(CreateSilenceFrame(samples_offset, samples_per_channel)));
    ASSERT_GE(OpusEncoder::kDtxMaxPacketSize, _encoded_frames.back().GetView().size);
}

std::vector<OpusCodecTestParams> MakeOpusCodecTestParams() {
    std::vector<OpusCodecTestParams> params;
    for(uint32_t channels : {1, 2}) {
        for(uint32_t sample_rate : {8000, 12000, 16000, 24000, 48000}) {
            params.push_back(OpusCodecTestParams{
                .sample_rate = sample_rate,
                .channels = channels
            });
        }
    }
    return params;
}

INSTANTIATE_TEST_SUITE_P(P, OpusCodecTest, ::testing::ValuesIn(MakeOpusCodecTestParams()));

}
