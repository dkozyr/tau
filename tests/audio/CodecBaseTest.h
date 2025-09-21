#include "tau/audio/OpusDecoder.h"
#include "tau/common/File.h"
#include "tests/lib/Common.h"

namespace tau::audio {

class CodecBaseTest {
public:
    CodecBaseTest()
        : _rtp_allocator(1500)
        , _frame_allocator(48000 * OpusDecoder::kDefaultFrameDurationMs / 1000 * sizeof(int16_t) * 2)
        , _pcm_data(ReadFile(std::string{PROJECT_SOURCE_DIR} + "/data/audio/16khz_mono_pcm_s16le.raw"))
        , _pcm_samples(reinterpret_cast<const int16_t*>(_pcm_data.data()))
        , _pcm_samples_count(_pcm_data.size() / sizeof(int16_t)) {
    }

protected:
    void InitParams(uint32_t sample_rate, uint32_t channels) {
        _sample_rate = sample_rate;
        _channels = channels;
    }

    Buffer CreateFrame(size_t samples_offset, size_t samples_per_channel) const {
        auto tp = SamplesOffsetToTimepoint(samples_offset);
        auto size = samples_per_channel * sizeof(int16_t) * _channels;
        auto frame = Buffer::Create(g_system_allocator, size, Buffer::Info{.tp = tp});
        auto view = frame.GetViewWithCapacity();
        if(samples_offset + samples_per_channel >= _pcm_samples_count) {
            samples_offset %= (_pcm_samples_count - samples_per_channel);
        }
        std::memcpy(view.ptr, _pcm_samples + samples_offset, view.size);
        frame.SetSize(view.size);
        return frame;
    }

    Buffer CreateSilenceFrame(size_t samples_offset, size_t samples_per_channel) const {
        auto tp = SamplesOffsetToTimepoint(samples_offset);
        auto size = samples_per_channel * sizeof(int16_t) * _channels;
        auto frame = Buffer::Create(g_system_allocator, size, Buffer::Info{.tp = tp});
        auto view = frame.GetViewWithCapacity();
        std::memset(view.ptr, 0, view.size);
        frame.SetSize(view.size);
        return frame;
    }

    Timepoint SamplesOffsetToTimepoint(size_t samples_offset) const {
        return static_cast<Timepoint>(samples_offset * kSec / _sample_rate / _channels);
    }

    uint32_t CalcEncodingBitrateKbps() const {
        EXPECT_FALSE(_encoded_frames.empty());

        auto bytes = 0;
        for(auto& frame : _encoded_frames) {
            bytes += frame.GetView().size;
        }
        auto tp_begin = _encoded_frames.front().GetInfo().tp;
        auto tp_end = _encoded_frames.back().GetInfo().tp;
        return static_cast<uint32_t>(bytes * 8 / DurationSec(tp_begin, tp_end) / 1000);
    }

    uint32_t CalcAverageEncodingPacketSize() const {
        auto bytes = 0;
        for(auto& frame : _encoded_frames) {
            bytes += frame.GetView().size;
        }
        return bytes ? bytes / _encoded_frames.size() : 0;
    }

    void DumpRawOutput(const std::string& output_file) {
        for(size_t i = 0; i < _decoded_frames.size(); ++i) {
            auto decoded = _decoded_frames[i].GetView();
            std::string_view data{reinterpret_cast<const char*>(decoded.ptr), decoded.size};
            WriteFile(output_file, data, i != 0);
        }
    }

    static constexpr size_t CalcFrameSamples(size_t frame_ms, size_t sample_rate) {
        return sample_rate * frame_ms / 1000;
    }

protected:
    PoolAllocator _rtp_allocator;
    PoolAllocator _frame_allocator;

    uint32_t _sample_rate = 48000;
    uint32_t _channels = 1;

    std::vector<Buffer> _encoded_frames;
    std::vector<Buffer> _decoded_frames;

    const std::string _pcm_data;
    const int16_t* _pcm_samples;
    const size_t _pcm_samples_count;
};

}
