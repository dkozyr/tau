#include "tau/stun/attribute/UseCandidate.h"
#include "tau/stun/AttributeType.h"

namespace tau::stun::attribute {

bool UseCandidateWriter::Write(Writer& writer) {
    if(writer.GetAvailableSize() < kAttributeHeaderSize) {
        return false;
    }
    writer.WriteAttributeHeader(AttributeType::kUseCandidate, 0);
    writer.UpdateHeaderLength();
    return true;
}

}
