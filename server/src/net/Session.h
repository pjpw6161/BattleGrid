#pragma once

#include "protocol/MessageDispatcher.h"

#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/system/error_code.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

namespace battlegrid
{
class Session : public std::enable_shared_from_this<Session>
{
public:
    Session(boost::asio::ip::tcp::socket socket, std::uint64_t playerId);

    void Start();

private:
    void OnAccept(const boost::system::error_code& error);
    void Read();
    void OnRead(const boost::system::error_code& error, std::size_t bytesTransferred);
    void OnWrite(const boost::system::error_code& error, std::size_t bytesTransferred);
    void LogError(std::string_view operation, const boost::system::error_code& error) const;

    boost::beast::websocket::stream<boost::asio::ip::tcp::socket> webSocket;
    boost::beast::flat_buffer buffer;
    std::string remoteEndpoint;
    std::uint64_t playerId;
    MessageDispatcher dispatcher;
    std::string outboundMessage;
};
}
