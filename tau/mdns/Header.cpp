#include "tau/mdns/Header.h"
#include "tau/common/NetToHost.h"

namespace tau::mdns {

uint16_t HeaderReader::GetId(const BufferViewConst& view) {
    return Read16(view.ptr);
}

uint16_t HeaderReader::GetFlags(const BufferViewConst& view) {
    return Read16(view.ptr + sizeof(uint16_t));
}

uint16_t HeaderReader::GetQuestionsCount(const BufferViewConst& view) {
    return Read16(view.ptr + 2 * sizeof(uint16_t));
}

uint16_t HeaderReader::GetAnswersCount(const BufferViewConst& view) {
    return Read16(view.ptr + 3 * sizeof(uint16_t));
}

uint16_t HeaderReader::GetAuthorityCount(const BufferViewConst& view) {
    return Read16(view.ptr + 4 * sizeof(uint16_t));
}

uint16_t HeaderReader::GetAdditionalCount(const BufferViewConst& view) {
    return Read16(view.ptr + 5 * sizeof(uint16_t));
}

bool HeaderReader::Validate(const BufferViewConst& view) {
    return view.size >= kHeaderSize;
}

bool HeaderWriter::Write(Writer& writer, uint16_t id, uint16_t flags, uint16_t questions, uint16_t answers,
                         uint16_t authority, uint16_t additional) {
    if(writer.GetAvailableSize() < kHeaderSize) {
        return false;
    }
    writer.Write(id);
    writer.Write(flags);
    writer.Write(questions);
    writer.Write(answers);
    writer.Write(authority);
    writer.Write(additional);
    return true;
}

}
