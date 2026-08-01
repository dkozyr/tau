#pragma once

#include "SessionState.h"

namespace tau::signalling {

struct SessionInfo {
    SessionId session_id;
    ClientId client_id;
    SessionState state;
};

}
