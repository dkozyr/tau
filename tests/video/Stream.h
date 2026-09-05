#include "tau/video/AnnexBParser.h"
#include <tau/memory/Buffer.h>
#include <tau/memory/SystemAllocator.h>
#include "tau/common/File.h"
#include "tau/common/Log.h"
#include <functional>
#include <functional>

namespace tau::video {

class Stream {
public:
    using Callback = std::function<void(Buffer&& nal_unit)>;

public:
    static void Process(const std::filesystem::path& path, Callback callback) {
        const auto raw_file = ReadFile(path);
        BufferViewConst raw_annexb_view{
            .ptr  = reinterpret_cast<const uint8_t*>(raw_file.data()),
            .size = raw_file.size()
        };

        Buffer::Info buffer_info{.tp = 0};
        etl::vector<BufferViewConst, 16> nal_units;
        while(true) {
            auto offset = ParseAnnexB(raw_annexb_view, nal_units);
            if(offset == 0) {
                break;
            }
            raw_annexb_view.ForwardPtrUnsafe(offset);

            TAU_LOG_DEBUG("Nal units: " << nal_units.size());
            for(auto& nal_unit : nal_units) {
                // TAU_LOG_INFO(" nal: " << nal_unit.size);
                // etl::string<256> dump;
                // TAU_LOG_INFO(ToHexDump(nal_unit.ptr, std::min<size_t>(nal_unit.size, 32), dump));

                auto buffer = tau::Buffer::Create(g_system_allocator, nal_unit, buffer_info);
                buffer_info.tp += 66 * kMs;

                callback(std::move(buffer));
            }
        }
    }

private:
    
};

}
