#include "tau/srtp/Common.h"
#include <srtp3/srtp.h>

namespace tau::srtp {

void Init() {
    srtp_init();
}

}
