#include "apps/rtsp-client/SetupUri.h"
#include <gtest/gtest.h>

namespace tau::rtsp {

TEST(SetupUriTest, AppendsCameraTrackToDescribeUri) {
    EXPECT_EQ(CreateSetupUri("rtsp://192.168.0.177:554/ch0_0.h264", {}, "track0"), "rtsp://192.168.0.177:554/ch0_0.h264/track0");
}

TEST(SetupUriTest, UsesContentBaseAndAvoidsDuplicateSlash) {
    EXPECT_EQ(CreateSetupUri("rtsp://camera/video", "rtsp://camera/stream/", "trackID=0"), "rtsp://camera/stream/trackID=0");
}

TEST(SetupUriTest, AbsoluteAndRootRelativeControl) {
    EXPECT_EQ(CreateSetupUri("rtsp://camera/video", {}, "rtsp://other/track"), "rtsp://other/track");
    EXPECT_EQ(CreateSetupUri("rtsp://camera/video", "rtsp://camera:554/stream/", "/track"), "rtsp://camera:554/track");
}

TEST(SetupUriTest, MissingOrAggregateControlUsesBase) {
    EXPECT_EQ(CreateSetupUri("rtsp://camera/video", {}, {}), "rtsp://camera/video");
    EXPECT_EQ(CreateSetupUri("rtsp://camera/video", "rtsp://camera/stream/", "*"), "rtsp://camera/stream/");
}

TEST(SetupUriTest, RejectsCapacityOverflow) {
    etl::string<256> base;
    base.resize(base.capacity(), 'x');
    EXPECT_THROW(CreateSetupUri(base, {}, "track"), std::runtime_error);
}

}
