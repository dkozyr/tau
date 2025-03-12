#include "tau/common/Event.h"

void Event::Set() {
    _done.set_value();
}

bool Event::IsSet() {
    return _done.get_future().valid();
}
