#include "apps/rtsp-client/SetupUri.h"
#include "tau/common/String.h"
#include "tau/common/Exception.h"

namespace tau::rtsp {

etl::string<256> CreateSetupUri(etl::string_view request_uri, etl::string_view content_base, etl::string_view control) {
    auto base = content_base.empty() ? request_uri : content_base;
    if(IsPrefix(control, "rtsp://") || IsPrefix(control, "rtsps://")) {
        base = control;
        control = {};
    }

    if(control == "*") {
        control = {};
    }

    if(!control.empty() && (control.front() == '/')) {
        const auto scheme = base.find("://");
        if(scheme == etl::string_view::npos) {
            TAU_EXCEPTION(std::runtime_error, "Invalid RTSP base URI");
        }
        base = base.substr(0, base.find('/', scheme + 3));
    }

    const bool separator = !base.empty() && !control.empty() && (base.back() != '/') && (control.front() != '/');
    etl::string<256> uri;
    if(base.size() + separator + control.size() > uri.capacity()) {
        TAU_EXCEPTION(std::runtime_error, "RTSP control URI too long");
    }

    uri.append(base);
    if(separator) {
        uri.push_back('/');
    }
    uri.append(control);
    return uri;
}

}
