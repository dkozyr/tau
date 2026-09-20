#pragma once

#include "apps/rtsp-client/Pipeline.h"
#include "tau/rtsp/Connection.h"
#include "tau/rtsp/Transport.h"

namespace tau::rtsp {

class SessionTcp {
public:
    struct Options {
        InterleavedChannels channels;
        Pipeline::Options pipeline;
    };

    using VideoCallback = Pipeline::VideoCallback;

public:
    SessionTcp(Connection& connection, Options&& options);

    void SetVideoCallback(VideoCallback callback);

private:
    void OnPacket(StreamReader::Message&& message);
    void SendRtcp(Buffer&& packet);

private:
    Connection& _connection;
    const InterleavedChannels _channels;

    Pipeline _pipeline;
};

}
