#pragma once

#include "tau/rtsp/Connection.h"
#include "tau/asio/Timer.h"
#include <array>
#include <etl/deque.h>
#include <etl/map.h>

namespace tau::rtsp {

class ConnectionState : public std::enable_shared_from_this<ConnectionState> {
public:
    ConnectionState(Executor executor, Connection::Options options);

    void SetPacketCallback(Connection::PacketCallback callback);

    std::future<StreamReader::Message> SendRequest(StreamReader::String request, etl::string_view cseq);
    std::future<void> SendPacket(uint8_t channel, StreamReader::String packet);

    void SetCloseCallback(std::function<void()> callback);
    void AsyncClose(std::function<void()> completion);
    void Close();

private:
    void Connect();
    void OnResolve(boost_ec ec, asio::ip::tcp::resolver::results_type endpoints);
    void OnConnect(boost_ec ec);

    void Read();
    void OnRead(boost_ec ec, size_t size);
    void OnMessage(StreamReader::Message&& message);

    void WriteNext();
    void OnWrite(boost_ec ec);

    void Fail(etl::string_view reason, boost_ec ec = {});

private:
    static constexpr size_t kQueueLimit = 32;

    struct Pending {
        explicit Pending(Executor executor);
        Timer timer;
        std::promise<StreamReader::Message> result;
    };

    struct Write {
        etl::string<StreamReader::kCapacity + 4> data;
        std::promise<void> result;
    };

private:
    const Connection::Options _options;

    Strand _strand;
    asio::ip::tcp::resolver _resolver;
    asio::ip::tcp::socket _socket;
    Timer _write_timer;

    StreamReader _reader;

    std::array<char, 4096> _input;
    using CSeq = etl::string<16>;
    etl::map<CSeq, std::shared_ptr<Pending>, kQueueLimit> _pending;
    etl::deque<std::shared_ptr<Write>, kQueueLimit> _writes;

    Connection::PacketCallback _packet_callback;
    std::function<void()> _close_callback;

    std::exception_ptr _failure;
    bool _connecting = false;
    bool _connected = false;
    bool _writing = false;
    bool _closed = false;
};

}
