#include "tests/audio/OpusCodecBaseTest.h"
#include "tests/audio/OpusCodecTestParams.h"

namespace tau::audio {

class OpusCodecManualTest : public OpusCodecBaseTest, public ::testing::Test {
public:
    ~OpusCodecManualTest() {
        TAU_LOG_INFO("Total encoded frames: " << _encoded_frames.size()
            << ", decoded frames: " << _decoded_frames.size()
            << ", encoding bitrate: " << CalcEncodingBitrateKbps()
            << " kbps, average packet size: " << CalcAverageEncodingPacketSize());
    }
};

TEST_F(OpusCodecManualTest, DISABLED_FecAndLoss_Mono) {
    _options.sample_rate = 16000;
    _options.channels = 1;
    _options.complexity = 10;
    _options.fec_loss_rate = 100;
    InitEncoder();
    InitDecoder(48000);

    size_t samples_offset = 0;
    auto samples_per_channel = CalcFrameSamples(20, _options.sample_rate);
    while(samples_offset + samples_per_channel <= _pcm_samples_count) {
        if(g_random.Int(0, 10) > 0) {
            ASSERT_TRUE(_encoder->Encode(CreateFrame(samples_offset, samples_per_channel)));
        }
        samples_offset += samples_per_channel * _options.channels;
    }
    TAU_LOG_INFO("Total encoded frames: " << _encoded_frames.size() << ", decoded frames: " << _decoded_frames.size());

    DumpRawOutput("./test_output_mono.raw");
}

TEST_F(OpusCodecManualTest, DISABLED_LowBitrate_Mono) {
    _options.sample_rate = 16000;
    _options.channels = 1;
    _options.bitrate = 12'000;
    _options.complexity = 5;
    _options.fec_loss_rate = 0;
    InitEncoder();
    InitDecoder(48000);

    size_t samples_offset = 0;
    auto samples_per_channel = CalcFrameSamples(20, _options.sample_rate);
    while(samples_offset + samples_per_channel <= _pcm_samples_count) {
        ASSERT_TRUE(_encoder->Encode(CreateFrame(samples_offset, samples_per_channel)));
        samples_offset += samples_per_channel * _options.channels;
    }

    DumpRawOutput("./test_output_mono.raw");
}

}
