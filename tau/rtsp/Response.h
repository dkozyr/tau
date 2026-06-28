#pragma once

#include "tau/rtsp/Header.h"

namespace tau::rtsp {

struct Response {
    size_t status_code;
    etl::string_view reason_phrase;
    Headers headers = {};
    etl::string_view body = {};
};

}
