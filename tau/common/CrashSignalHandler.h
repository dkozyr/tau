#include <stdio.h>
#include <execinfo.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#include <array>
#include <atomic>
#include <iostream>

#include <boost/stacktrace.hpp>

namespace tau {

inline std::atomic_bool g_crash_signal_handler_processed{false};

inline void DumpStackTraceOnCrashSignal(int signal) {
    if(g_crash_signal_handler_processed.exchange(true) == true) {
        return;
    }

    // std::array<void*, 16> stack_entries;
    // auto size = backtrace(stack_entries.data(), stack_entries.size());
    // fprintf(stderr, "Error: signal %d (%s):\n", signal, strsignal(signal));
    // backtrace_symbols_fd(stack_entries.data(), size, STDERR_FILENO);

    std::cout << "Signal: " << signal << std::endl;
    std::cout << "Boost stacktrace:" << std::endl;
    std::cout << boost::stacktrace::stacktrace();

    std::fflush(stdout);
    std::fflush(stderr);
    exit(1);
}

class CrashSignalHandler {
public:
    CrashSignalHandler() {
        signal(SIGSEGV, DumpStackTraceOnCrashSignal);
        signal(SIGABRT, DumpStackTraceOnCrashSignal);
        signal(SIGBUS, DumpStackTraceOnCrashSignal);
    }

    ~CrashSignalHandler() {
        signal(SIGSEGV, SIG_DFL);
        signal(SIGABRT, SIG_DFL);
        signal(SIGBUS, SIG_DFL);
    }
};

}
