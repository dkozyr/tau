#include "tau/asio/ThreadPool.h"
#include "tests/Common.h"

namespace tau {

class ThreadPoolTest : public ::testing::Test {
public:
    struct Output {
        std::vector<size_t> data;

        void Push(size_t value) {
            data.push_back(value);
        }

        size_t GetSize() const {
            return data.size();
        }
    };

protected:
    static void AssertReordering(const std::vector<size_t>& data, bool target) {
        ASSERT_LT(1, data.size());
        bool reordered = false;
        for(size_t i = 1; i < data.size(); ++i) {
            if(data[i - 1] > data[i]) {
                reordered = true;
                break;
            }
        }
        ASSERT_EQ(target, reordered);
    }
};

TEST_F(ThreadPoolTest, Basic) {
    ThreadPool io(std::thread::hardware_concurrency());
    auto executor = io.GetExecutor();

    Output output;
    std::mutex mutex;
    const size_t kTestValues = 10;
    for(size_t i = 0; i < 10; ++i) {
        asio::post(executor, [i, &output, &mutex]() {
            LOG_INFO << "i: " << i;
            std::lock_guard lock(mutex);
            output.Push(i);
        });
    }
    {
        std::lock_guard lock(mutex);
        ASSERT_GT(kTestValues, output.GetSize());
    }

    io.Join();
    ASSERT_EQ(kTestValues, output.GetSize());
    ASSERT_NO_FATAL_FAILURE(AssertReordering(output.data, true));
}

TEST_F(ThreadPoolTest, Strand) {
    ThreadPool io(std::thread::hardware_concurrency());
    auto executor = io.GetStrand();

    Output output;
    const size_t kTestValues = 10;
    for(size_t i = 0; i < 10; ++i) {
        asio::post(executor, [i, &output]() {
            LOG_INFO << "i: " << i;
            output.Push(i); // mutex not needed with strand
        });
    }

    io.Join();
    ASSERT_EQ(kTestValues, output.GetSize());
    ASSERT_NO_FATAL_FAILURE(AssertReordering(output.data, false));
}

}
