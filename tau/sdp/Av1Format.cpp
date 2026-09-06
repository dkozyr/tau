#include <tau/sdp/Av1Format.h>
#include <tau/common/String.h>

namespace tau::sdp {

std::optional<Av1Format> ParseAv1Format(etl::string_view format) {
    Av1Format result;
    size_t pos = 0;
    while(pos != etl::string_view::npos) {
        auto token = SplitNext(format, pos, ";");
        SplitTokens<3> parts;
        Split(parts, token, "=");
        if(parts.size() == 2) {
            const auto name = Trim(parts[0]);
            const auto value = StringToUnsigned<uint8_t>(Trim(parts[1]));
            if(value) {
                if(Equal(name, "profile")) {
                    result.profile = *value;
                } else if(Equal(name, "level-idx")) {
                    result.level_index = *value;
                } else if(Equal(name, "tier")) {
                    result.tier = *value;
                }
            }
        }
    }
    if((result.profile > 2) || (result.tier > 1) ||
       ((result.level_index > 23) && (result.level_index != 31)) ||
       ((result.tier == 1) && (result.level_index < 8))) {
        return std::nullopt;
    }
    return result;
}

Codec::Format CreateAv1Format(const Av1Format& format) {
    Codec::Format result;
    etl::string_stream ss(result);
    ss << "profile=" << (size_t)format.profile << ";level-idx=" << (size_t)format.level_index << ";tier=" << (size_t)format.tier;
    return result;
}

}
