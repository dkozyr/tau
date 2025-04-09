#pragma once

namespace tau::ice {

enum State {
    kWaiting,
    kRunning,
    kReady,     // on first valid pair
    kCompleted,
    kFailed
};

}
