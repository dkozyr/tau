#include "tau/audio/OpusDecoder.h"
#include "tau/common/Exception.h"
#include "tau/common/Log.h"
#include <algorithm>

namespace tau::audio {

OpusDecoder::OpusDecoder(Allocator& allocator, Options&& options)
    : _allocator(allocator)
    , _options(std::move(options))
    , _target_frame_samples_per_channel(CalcFrameSamplesPerChannel(_options, _allocator.GetChunkSize()))
    , _decode_buffer(SamplesToBytes(_options.sample_rate * kMaxFrameDurationMs / 1000))
    , _decode_buffer_samples_per_channel(_decode_buffer.size() / sizeof(int16_t) / _options.channels) {
    TAU_LOG_INFO("Options: sample_rate: " << _options.sample_rate << ", channels: " << _options.channels << ", target samples per channel: " << _target_frame_samples_per_channel << ", buffer size: " << _decode_buffer.size());

    int error;
    _decoder = opus_decoder_create(_options.sample_rate, _options.channels, &error);
    if(error < 0) {
        TAU_EXCEPTION(std::runtime_error, "Failed to create a decoder: " << opus_strerror(error));
    }
}

OpusDecoder::~OpusDecoder() {
    opus_decoder_destroy(_decoder);
}

bool OpusDecoder::Decode(Buffer&& frame) {
    BufferViewConst frame_view = ToConst(frame.GetView()); 
    return Decode(frame_view, frame.GetInfo().tp);
}
bool OpusDecoder::Decode(const BufferViewConst& view, Timepoint tp) {
    auto view_size = static_cast<int32_t>(view.size);
    auto samples_per_channel = opus_decoder_get_nb_samples(_decoder, view.ptr, view_size);
    if(samples_per_channel <= 0) {
        TAU_LOG_WARNING_THR(256, "Failed to get frame samples per channel: " << samples_per_channel << ", error: " << opus_strerror(samples_per_channel));
        return false;
    }

    int decoded_samples_per_channel = opus_decode(_decoder, view.ptr, view_size, reinterpret_cast<int16_t*>(_decode_buffer.data()), _decode_buffer.size(), 0);
    if(decoded_samples_per_channel < 0) {
        TAU_LOG_WARNING_THR(256, "Failed to decode a frame: " << opus_strerror(decoded_samples_per_channel));
        return false;
    }
    if(samples_per_channel != decoded_samples_per_channel) {
        TAU_LOG_WARNING_THR(256, "Decoded samples per channel mismatch: " << decoded_samples_per_channel << " != " << samples_per_channel);
        return false;
    }

    size_t offset = 0;
    _tp = tp;
    while(decoded_samples_per_channel > 0) {
        auto frame_samples_per_channel = std::min<size_t>(decoded_samples_per_channel, _target_frame_samples_per_channel);
        auto frame_size = SamplesToBytes(frame_samples_per_channel);

        auto decoded_frame = Buffer::Create(_allocator, frame_size, Buffer::Info{.tp = _tp});
        auto decoded_view = decoded_frame.GetViewWithCapacity();
        std::memcpy(decoded_view.ptr, _decode_buffer.data() + offset, frame_size);
        decoded_frame.SetSize(frame_size);
        _callback(std::move(decoded_frame));

        decoded_samples_per_channel -= frame_samples_per_channel;
        offset += frame_size;
        _tp += static_cast<Timepoint>(frame_samples_per_channel * kSec / _options.sample_rate);
    }
    return true;
}

bool OpusDecoder::DecodePlc(Timepoint tp) {
    if(_tp >= tp) {
        return true;
    }
    if(_tp + kMaxPlcDurationMs * kMs < tp) {
        TAU_LOG_WARNING_THR(256, "Too big packet loss concealment period, _tp: " << DurationMsInt(_tp) << " ms, tp: " << DurationMsInt(tp) << " ms");
        return false;
    }
    while(_tp < tp) {
        int frame_samples_per_channel = std::min<int>(_target_frame_samples_per_channel, (tp - _tp) * _options.sample_rate / kSec);
        int decoded_samples_per_channel = opus_decode(_decoder, nullptr, 0, reinterpret_cast<int16_t*>(_decode_buffer.data()), frame_samples_per_channel, 1);
        if(decoded_samples_per_channel < 0) {
            TAU_LOG_WARNING_THR(256, "Failed to decode PLC: " << opus_strerror(decoded_samples_per_channel));
            return false;
        }

        auto frame_size = SamplesToBytes(frame_samples_per_channel);
        auto decoded_frame = Buffer::Create(_allocator, frame_size, Buffer::Info{.tp = _tp});
        auto decoded_view = decoded_frame.GetViewWithCapacity();
        std::memcpy(decoded_view.ptr, _decode_buffer.data(), frame_size);
        decoded_frame.SetSize(frame_size);
        _callback(std::move(decoded_frame));

        _tp += static_cast<Timepoint>(frame_samples_per_channel * kSec / _options.sample_rate);
    }
    return true;
}

uint32_t OpusDecoder::CalcFrameSamplesPerChannel(const Options& options, uint32_t frame_size) {
    auto samples = frame_size / sizeof(int16_t) / options.channels;
    auto default_frame_samples = (options.sample_rate * options.frame_duration_ms) / 1000;
    return std::min<uint32_t>(samples, default_frame_samples);
}

}
