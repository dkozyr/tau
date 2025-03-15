#pragma once

#include "tau/memory/BufferView.h"

namespace tau::rtp {

class Writer {
public:
    struct Options{
        uint8_t pt;
        uint32_t ssrc;
        uint32_t ts;
        uint16_t sn;
        bool marker; //TODO: false by default?
        uint16_t extension_length_in_words = 0;
    };

    struct Result{
        size_t size = 0;
        BufferView payload = kBufferViewNull;
        BufferView extension = kBufferViewNull;
    };

public:
    static Result Write(BufferView view_with_capacity, const Options& options);
    static bool AddPadding(BufferView view_with_capacity, uint8_t padding_size);
};

}
