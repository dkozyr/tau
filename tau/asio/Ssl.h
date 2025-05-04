#pragma once

#include "tau/asio/Common.h"
#include <boost/asio/ssl.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <vector>

namespace tau {

namespace asio_ssl = asio::ssl;
using SslContext    = asio_ssl::context;
using SslSocket     = asio_ssl::stream<asio_tcp::socket>;
using SslContextPtr = std::unique_ptr<SslContext>;
using SslSocketPtr  = std::shared_ptr<SslSocket>;

SslContext CreateSslContext(const std::vector<uint8_t>& cert, const std::vector<uint8_t>& key);
SslContextPtr CreateSslContextPtr(const std::vector<uint8_t>& cert, const std::vector<uint8_t>& key);

}
