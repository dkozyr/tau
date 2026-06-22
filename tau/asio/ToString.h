#pragma once

#include "tau/asio/Common.h"
#include <etl/string_stream.h>

namespace tau {

inline etl::string_stream& operator<<(etl::string_stream& ss, boost_ec ec) {
    return ss << ec.value() << ", " << ec.message().c_str();
}

inline etl::string_stream& operator<<(etl::string_stream& ss, const asio::ip::address& address) {
    const auto ipv4 = address.to_v4().to_uint();
    ss << ((ipv4 >> 24) & 0xFF) << "."
       << ((ipv4 >> 16) & 0xFF) << "."
       << ((ipv4 >> 8)  & 0xFF) << "."
       << ((ipv4)       & 0xFF);
    return ss;
}

inline etl::string_stream& operator<<(etl::string_stream& ss, const asio::ip::tcp::endpoint& endpoint) {
    return ss << endpoint.address() << ":" << endpoint.port();
}

inline etl::string_stream& operator<<(etl::string_stream& ss, const beast_request& request) {
    for(const auto& chunk : request.body().data()) {
        ss << etl::string_view{static_cast<const char*>(chunk.data()), chunk.size()};
    }
    return ss;
}

}
