#include "tests/webrtc/CallContext.h"
#include "tau/asio/ThreadPool.h"
#include "tests/lib/Common.h"

namespace tau::webrtc {

class PeerConnectionBaseTest {
public:
    PeerConnectionBaseTest()
        : _io(std::thread::hardware_concurrency())
    {}

    ~PeerConnectionBaseTest() {
        _io.Join();
    }

protected:
    ClientContext::Dependencies CreatePcDependencies() {
        return ClientContext::Dependencies{
            .clock = _clock,
            .executor = _io.GetStrand(),
            .udp_allocator = g_udp_allocator
        };
    }

protected:
    ThreadPool _io;
    SteadyClock _clock;
};

}
