#pragma once

#include <tau/net/Endpoint.h>
#include <optional>

namespace tau::net {

std::optional<Endpoint> ResolveEndpointV4(const etl::string_view& host, const etl::string_view& service = {});

}
