#pragma once

#include "protocol/MessageDispatcher.h"

#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/system/error_code.hpp>

#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <string>
#include <string_view>

namespace battlegrid
{
class RoomManager;

class Session : public std::enable_shared_from_this<Session>
{
public:
    Session(
        boost::asio::ip::tcp::socket socket,
        std::shared_ptr<RoomManager> roomManager,
        std::function<std::uint64_t()> allocatePlayerId
    );

    void Start();
    void SendText(const std::string& message);
    bool IsJoined() const;
    std::uint64_t GetPlayerId() const;
    std::uint64_t GetRoomId() const;
    bool MarkMatchEndedSent(int matchId);

private:
    void OnAccept(const boost::system::error_code& error);
    void Read();
    void OnRead(const boost::system::error_code& error, std::size_t bytesTransferred);
    void DoWrite();
    void OnWrite(const boost::system::error_code& error, std::size_t bytesTransferred);
    void HandleDisconnect();
    void LogError(std::string_view operation, const boost::system::error_code& error) const;

    boost::beast::websocket::stream<boost::asio::ip::tcp::socket> webSocket;
    boost::beast::flat_buffer buffer;
    std::string remoteEndpoint;
    SessionState sessionState;
    std::shared_ptr<RoomManager> roomManager;
    std::function<std::uint64_t()> allocatePlayerId;
    std::deque<std::string> outgoingMessages;
    int lastMatchEndedMatchIdSent;
};
}
