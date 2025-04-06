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

using IpAddress = asio_ip::address;
using IpAddressV4 = asio_ip::address_v4;
using IpAddressV6 = asio_ip::address_v6;
using Endpoint = asio_udp::endpoint;

}
