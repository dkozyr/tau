#include "tau/srtp/Common.h"
#include "tau/common/Log.h"
#include <srtp3/srtp.h>

namespace tau::srtp {

void Init() {
    if(auto error = srtp_init()) {
        TAU_LOG_ERROR("srtp_init failed, error: " << error);
    }
}

}
