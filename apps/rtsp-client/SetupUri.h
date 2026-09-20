#pragma once

#include <etl/string.h>
#include <etl/string_view.h>

namespace tau::rtsp {

etl::string<256> CreateSetupUri(etl::string_view request_uri, etl::string_view content_base, etl::string_view control);

}
