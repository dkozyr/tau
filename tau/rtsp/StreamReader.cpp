#include "tau/rtsp/StreamReader.h"
#include "tau/rtsp/Header.h" //TODO: move kClRfClRf to other file
#include "tau/common/String.h"
#include "tau/common/Exception.h"

namespace tau::rtsp {

StreamReader::StreamReader()
    : StreamReader(Options{})
{}

StreamReader::StreamReader(Options options)
    : _options(options) {
    if((options.max_header_size < 4) || (options.max_header_size > kCapacity) ||
       (options.max_packet_size == 0) || (options.max_packet_size > 65535)) {
        TAU_EXCEPTION(std::invalid_argument, "Invalid RTSP stream limits");
    }
}

bool StreamReader::Push(etl::string_view data, const Callback& callback) {
    if(_failed) {
        return false;
    }

    for(const auto character : data) {
        if(_buffer.full()) {
            _failed = true;
            return false;
        }
        _buffer.push_back(character);

        if(!_message_size) {
            if(!ParseMessageSize()) {
                return false;
            }
        }

        if(_message_size && (_buffer.size() == *_message_size)) {
            Message message{
                .channel = _channel,
                .data = std::move(_buffer)
            };
            _channel.reset();
            _buffer.clear();
            _message_size.reset();
            callback(std::move(message));
        }
    }
    return true;
}

bool StreamReader::Finish() const {
    return !_failed && _buffer.empty() && !_message_size;
}

void StreamReader::Reset() {
    _buffer.clear();
    _channel.reset();
    _message_size.reset();
    _failed = false;
}

bool StreamReader::ParseMessageSize() {
    const auto interleaved = _channel.has_value() || (_buffer[0] == '$');
    if(interleaved) {
        if(_buffer.size() < kFrameHeaderSize) {
            return true;
        }

        const auto length = static_cast<uint8_t>(_buffer[2]) * kLengthByteBase + static_cast<uint8_t>(_buffer[3]);
        if((length == 0) || (length > _options.max_packet_size)) {
            _failed = true;
            return false;
        }
        _channel = static_cast<uint8_t>(_buffer[1]);
        _buffer.clear();
        _message_size = length;
    } else {
        if((_buffer.size() >= kFrameHeaderSize) && (_buffer.compare(_buffer.size() - kFrameHeaderSize, kFrameHeaderSize, kClRfClRf) == 0)) {
            if(!ReadBodySize()) {
                _failed = true;
                return false;
            }
        } else if(_buffer.size() >= _options.max_header_size) {
            _failed = true;
            return false;
        }
    }
    return true;
}

bool StreamReader::ReadBodySize() {
    const etl::string_view headers{_buffer.data(), _buffer.size()};

    size_t pos = 0;
    std::optional<size_t> body_size;
    while(pos != etl::string_view::npos) {
        auto line = SplitNext(headers, pos, kClRf);
        if((line == kClRf) || line.empty()) {
            break;
        }
        const bool first_line = (pos == line.size() + kClRf.size());
        if(first_line) {
            continue;
        }

        const auto separator = line.find(':');
        if(separator == etl::string_view::npos) {
            return false;
        }

        const auto name = Trim(line.substr(0, separator));
        if(name.empty()) {
            return false;
        }

        if(Equal(name, "Content-Length")) {
            const auto value = Trim(line.substr(separator + 1));
            if(body_size || value.empty()) {
                return false;
            }

            size_t length = 0;
            for(const auto character : value) {
                if(!IsDigit(character)) {
                    return false;
                }
                const auto digit = static_cast<size_t>(character - '0');
                if(length > (kCapacity - digit) / 10) {
                    return false;
                }
                length = length * 10 + digit;
            }
            body_size = length;
        }
    }
    if(body_size.value_or(0) > kCapacity - _buffer.size()) {
        return false;
    }
    _message_size = _buffer.size() + body_size.value_or(0);
    return true;
}

}
