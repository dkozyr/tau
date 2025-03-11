#pragma once

#include <boost/asio.hpp>
#include <boost/asio/strand.hpp>

namespace asio = boost::asio;
using Executor = asio::any_io_executor;
