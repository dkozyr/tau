#include "tau/audio/OpusEncoder.h"
#include "tau/common/Exception.h"
#include "tau/common/Log.h"

namespace tau::audio {

OpusEncoder::OpusEncoder(Allocator& allocator, Options&& options)
    : _allocator(allocator)
    , _options(std::move(options)) {
    TAU_LOG_INFO("Options: sample_rate: " << _options.sample_rate << ", channels: " << _options.channels
        << ", bitrate: " << _options.bitrate << ", complexity: " << _options.complexity
        << ", fec_loss_rate: " << _options.fec_loss_rate << ", dtx: " << _options.dtx);

    int error;
    _encoder = opus_encoder_create(_options.sample_rate, _options.channels, OPUS_APPLICATION_VOIP, &error);
    if(error < 0) {
        TAU_EXCEPTION(std::runtime_error, "Failed to create an encoder: " << opus_strerror(error));
    }

    error = opus_encoder_ctl(_encoder, OPUS_SET_COMPLEXITY(_options.complexity));
    if(error < 0) {
        TAU_LOG_WARNING("Failed to set complexity: " << _options.complexity << ", error: " << opus_strerror(error));
    }

    error = opus_encoder_ctl(_encoder, OPUS_SET_BITRATE(_options.bitrate));
    if(error < 0) {
        TAU_LOG_WARNING("Failed to set bitrate: " << _options.bitrate << ", error: " << opus_strerror(error));
    }

    const auto fec = (_options.fec_loss_rate > 0);
    if(fec) {
        error = opus_encoder_ctl(_encoder, OPUS_SET_INBAND_FEC(1));
        if(error < 0) {
            TAU_LOG_WARNING("Failed to set inband FEC: " << fec << ", error: " << opus_strerror(error));
        }

        error = opus_encoder_ctl(_encoder, OPUS_SET_PACKET_LOSS_PERC(_options.fec_loss_rate));
        if(error < 0) {
            TAU_LOG_WARNING("Failed to set packet loss percentage: " << _options.fec_loss_rate << ", error: " << opus_strerror(error));
        }
    }

    if(_options.dtx) {
        error = opus_encoder_ctl(_encoder, OPUS_SET_DTX(1));
        if(error < 0) {
            TAU_LOG_WARNING("Failed to enable DTX, error: " << opus_strerror(error));
        }
    }

    // error = opus_encoder_ctl(_encoder, OPUS_SET_SIGNAL(OPUS_SIGNAL_VOICE));
    // if(error < 0) {
    //     TAU_LOG_WARNING("Failed to set signal: " << OPUS_SIGNAL_VOICE << ", error: " << opus_strerror(error));
    // }

    // error = opus_encoder_ctl(_encoder, OPUS_SET_MAX_BANDWIDTH(OPUS_BANDWIDTH_NARROWBAND));
    // if(error < 0) {
    //     TAU_LOG_WARNING("Failed to set max bandwidth: " << OPUS_BANDWIDTH_NARROWBAND << ", error: " << opus_strerror(error));
    // }
}

OpusEncoder::~OpusEncoder() {
    opus_encoder_destroy(_encoder);
}

bool OpusEncoder::Encode(const Buffer& frame) {
    auto view = frame.GetView();
    auto samples_per_channel = view.size / (sizeof(int16_t) * _options.channels);
    if(samples_per_channel <= 0) {
        TAU_LOG_WARNING_THR(256, "Invalid frame samples per channel: " << samples_per_channel);
        return false;
    }

    auto encoded = Buffer::Create(_allocator, frame.GetInfo());
    auto encoded_view = encoded.GetViewWithCapacity();

    auto pcm = reinterpret_cast<const int16_t*>(view.ptr);
    auto encoded_size = opus_encode(_encoder, pcm, samples_per_channel, encoded_view.ptr, encoded_view.size);
    if(encoded_size < 0) {
        TAU_LOG_WARNING_THR(256, "Failed to encode a frame: " << opus_strerror(encoded_size));
        return false;
    }

    encoded.SetSize(static_cast<size_t>(encoded_size));
    _callback(std::move(encoded));
    return true;
}

bool OpusEncoder::SetLossRate(uint32_t loss_rate) {
    const auto fec = (loss_rate > 0);
    auto error = opus_encoder_ctl(_encoder, OPUS_SET_INBAND_FEC(fec ? 1 : 0));
    if(error < 0) {
        TAU_LOG_WARNING("Failed to set inband FEC: " << fec << ", error: " << opus_strerror(error));
    }

    error = opus_encoder_ctl(_encoder, OPUS_SET_PACKET_LOSS_PERC(loss_rate));
    if(error < 0) {
        TAU_LOG_WARNING("Failed to set packet loss percentage: " << loss_rate << ", error: " << opus_strerror(error));
    }
    return true;
}

}
