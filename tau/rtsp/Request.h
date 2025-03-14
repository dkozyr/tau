#pragma once

#include "tau/rtsp/Header.h"

namespace rtsp {

enum Method {
    kOptions,
    kDescribe,
    kSetup,
    kPlay,
    kTeardown
};

struct Request {
    std::string uri;
    Method method;
    Headers headers = {};
};

}
