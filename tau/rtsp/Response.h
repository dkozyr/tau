#pragma once

#include "tau/rtsp/Header.h"

namespace rtsp {

struct Response {
    size_t status_code;
    std::string reason_phrase;
    Headers headers = {};
    std::string body = {};
};

}
