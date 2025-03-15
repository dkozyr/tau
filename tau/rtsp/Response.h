#pragma once

#include "tau/rtsp/Header.h"

namespace tau::rtsp {

struct Response {
    size_t status_code;
    std::string reason_phrase;
    Headers headers = {};
    std::string body = {};
};

}
