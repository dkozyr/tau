#include <tau/common/Clock.h>
#include <sstream>
#include <ctime>

namespace tau {

double DurationSec(Timepoint a, Timepoint b) {
    return static_cast<double>(b - a) * 1e-9;
}

double DurationSec(Timepoint a) {
    return static_cast<double>(a) * 1e-9;
}

double DurationMs(Timepoint a, Timepoint b) {
    return (b - a) / kMs;
}

double DurationMs(Timepoint a) {
    return a / kMs;
}

int64_t DurationMsInt(Timepoint a, Timepoint b) {
    return static_cast<int64_t>((b - a) / kMs);
}

int64_t DurationMsInt(Timepoint a) {
    return static_cast<int64_t>(a / kMs);
}

Timepoint FromIso8601(const std::string& iso) {
    std::tm tm = {};
    std::istringstream ss(iso);
    ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    if(ss.fail()) {
        return 0;
    }
    time_t time = timegm(&tm);
    return static_cast<Timepoint>(time) * kSec;
}

}
