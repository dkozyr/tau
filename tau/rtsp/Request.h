#pragma once

#include "tau/rtsp/Header.h"

namespace tau::rtsp {

enum Method {
    kOptions,
    kDescribe,
    kSetup,
    kPlay,
    kTeardown
};

struct Request {
    etl::string_view uri;
    Method method;
    Headers headers = {};
};

}
