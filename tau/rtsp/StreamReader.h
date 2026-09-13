#pragma once

#include <etl/string.h>
#include <etl/string_view.h>
#include <functional>
#include <optional>
#include <cstdint>
#include <cstddef>

namespace tau::rtsp {

class StreamReader {
public:
    static constexpr size_t kCapacity = 65536;
    using String = etl::string<kCapacity>;

    struct Options {
        size_t max_header_size = 16 * 1024;
        size_t max_packet_size = 65535;
    };

    struct Message {
        std::optional<uint8_t> channel;
        String data;
    };

    using Callback = std::function<void(Message&&)>;

public:
    StreamReader();
    explicit StreamReader(Options options);

    bool Push(etl::string_view data, const Callback& callback);
    bool Finish() const;
    void Reset();

private:
    bool ParseMessageSize();
    bool ReadBodySize();

private:
    static constexpr size_t kFrameHeaderSize = 4;
    static constexpr size_t kLengthByteBase = 256;

private:
    const Options _options;
    String _buffer;
    std::optional<uint8_t> _channel;
    std::optional<size_t> _message_size;
    bool _failed = false;
};

}
