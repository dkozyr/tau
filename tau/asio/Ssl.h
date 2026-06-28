#pragma once

#include "tau/asio/Common.h"
#include <boost/asio/ssl.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <etl/vector.h>

namespace tau {

namespace asio_ssl = asio::ssl;
using SslContext    = asio_ssl::context;
using SslSocket     = asio_ssl::stream<asio::ip::tcp::socket>;
using SslContextPtr = std::unique_ptr<SslContext>;
using SslSocketPtr  = std::shared_ptr<SslSocket>;

SslContext CreateSslContext(const etl::ivector<uint8_t>& cert, const etl::ivector<uint8_t>& key);
SslContextPtr CreateSslContextPtr(const etl::ivector<uint8_t>& cert, const etl::ivector<uint8_t>& key);

}
