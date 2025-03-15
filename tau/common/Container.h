#pragma once

namespace tau {

template<typename T, typename TValue>
bool Contains(const T& container, const TValue& value) {
    return container.find(value) != container.end();
}

}
