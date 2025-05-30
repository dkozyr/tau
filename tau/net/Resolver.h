#pragma once

#include "tau/asio/Common.h"
#include <optional>

namespace tau::net {

using Resolver = asio_tcp::resolver;
using ResolverResults = Resolver::results_type;

std::optional<ResolverResults> Resolve(Executor executor, const std::string& host, uint16_t port = 0);

}
