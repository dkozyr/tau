#pragma once

#include <boost/asio.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/system/error_code.hpp>
#include <functional>
#include <memory>

namespace tau {

namespace asio = boost::asio;
using Executor = asio::any_io_executor;
using boost_ec = boost::system::error_code;

namespace asio_ip = asio::ip;
using asio_udp = asio_ip::udp;
using asio_tcp = asio_ip::tcp;

}
