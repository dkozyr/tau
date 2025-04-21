#pragma once

#include "tau/sdp/Sdp.h"
#include "tau/sdp/Reader.h"
#include "tests/lib/Common.h"

namespace tau::sdp {

class ReaderWriterBase {
protected:
    static void AssertSdp(const Sdp& target, const Sdp& actual) {
        ASSERT_EQ(target.bundle_mids.size(), actual.bundle_mids.size());
        for(size_t i = 0; i < target.bundle_mids.size(); ++i) {
            ASSERT_EQ(target.bundle_mids[i], actual.bundle_mids[i]);
        }
        ASSERT_NO_FATAL_FAILURE(AssertIce(target.ice, actual.ice));
        ASSERT_NO_FATAL_FAILURE(AssertDtls(target.dtls, actual.dtls));
        ASSERT_EQ(target.medias.size(), actual.medias.size());
        for(size_t i = 0; i < target.medias.size(); ++i) {
            ASSERT_NO_FATAL_FAILURE(AssertMedia(target.medias[i], actual.medias[i]));
        }
    }

    static void AssertMedia(const Media& target, const Media& actual) {
        ASSERT_EQ(target.type,      actual.type);
        ASSERT_EQ(target.mid,       actual.mid);
        ASSERT_EQ(target.direction, actual.direction);
        ASSERT_EQ(target.codecs.size(), actual.codecs.size());
        for(auto& [pt, codec] : target.codecs) {
            ASSERT_NO_FATAL_FAILURE(AssertCodec(codec, actual.codecs.at(pt)));
        }
    }

    static void AssertCodec(const Codec& target, const Codec& actual) {
        ASSERT_EQ(target.index,      actual.index);
        ASSERT_EQ(target.name,       actual.name);
        ASSERT_EQ(target.clock_rate, actual.clock_rate);
        ASSERT_EQ(target.rtcp_fb,    actual.rtcp_fb);
        ASSERT_EQ(target.format,     actual.format);
    }

    static void AssertIce(const std::optional<Ice>& target, const std::optional<Ice>& actual) {
        if(target.has_value()) {
            ASSERT_TRUE(actual.has_value());
            ASSERT_EQ(target->trickle,           actual->trickle);
            ASSERT_EQ(target->ufrag,             actual->ufrag);
            ASSERT_EQ(target->pwd,               actual->pwd);
            ASSERT_EQ(target->candidates.size(), actual->candidates.size());
            for(size_t i = 0; i < target->candidates.size(); ++i) {
                ASSERT_EQ(target->candidates[i], actual->candidates[i]);
            }
        } else {
            ASSERT_FALSE(actual.has_value());
        }
    }

    static void AssertDtls(const std::optional<Dtls>& target, const std::optional<Dtls>& actual) {
        if(target.has_value()) {
            ASSERT_TRUE(actual.has_value());
            ASSERT_EQ(target->fingerprint_sha256, actual->fingerprint_sha256);
        } else {
            ASSERT_FALSE(actual.has_value());
        }
    }
};

}
