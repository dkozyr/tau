#pragma once

#include "tau/rtsp/Header.h"

namespace rtsp {

struct Response {
    size_t status_code;
    std::string reason_phrase;
    size_t cseq;
    std::vector<Header> headers = {};
    std::string body = {};
};

}
