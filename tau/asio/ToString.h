#pragma once

#include "tau/asio/Common.h"
#include <etl/string_stream.h>

namespace etl {

inline etl::string_stream& operator<<(etl::string_stream& ss, boost::system::error_code ec) {
    return ss << ec.value() << ", " << ec.message().c_str();
}

inline etl::string_stream& operator<<(etl::string_stream& ss, const boost::asio::ip::address& address) {
    const auto ipv4 = address.to_v4().to_uint();
    ss << ((ipv4 >> 24) & 0xFF) << "."
       << ((ipv4 >> 16) & 0xFF) << "."
       << ((ipv4 >> 8)  & 0xFF) << "."
       << ((ipv4)       & 0xFF);
    return ss;
}

template <typename TProtocol>
inline etl::string_stream& operator<<(etl::string_stream& ss, const boost::asio::ip::basic_endpoint<TProtocol>& endpoint) {
    return ss << endpoint.address() << ":" << endpoint.port();
}

inline etl::string_stream& operator<<(etl::string_stream& ss, const boost::beast::http::request<boost::beast::http::dynamic_body>& request) {
    for(const auto& chunk : request.body().data()) {
        ss << etl::string_view{static_cast<const char*>(chunk.data()), chunk.size()};
    }
    return ss;
}

}
