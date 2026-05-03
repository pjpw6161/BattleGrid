#include "net/Session.h"

#include "core/Logger.h"

#include <boost/asio/buffer.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/core/buffers_to_string.hpp>
#include <boost/beast/http.hpp>

#include <string>
#include <utility>

namespace battlegrid
{
namespace
{
using boost::asio::ip::tcp;
namespace beast = boost::beast;
namespace websocket = boost::beast::websocket;

std::string EndpointToString(const tcp::endpoint& endpoint)
{
    return endpoint.address().to_string() + ":" + std::to_string(endpoint.port());
}
}

Session::Session(tcp::socket socket, std::uint64_t sessionPlayerId)
    : webSocket(std::move(socket)),
      buffer(),
      remoteEndpoint("unknown"),
      playerId(sessionPlayerId),
      dispatcher(sessionPlayerId),
      outboundMessage()
{
    boost::system::error_code error;
    const tcp::endpoint endpoint = webSocket.next_layer().remote_endpoint(error);
    if (!error)
    {
        remoteEndpoint = EndpointToString(endpoint);
    }
}

void Session::Start()
{
    webSocket.set_option(websocket::stream_base::timeout::suggested(beast::role_type::server));
    webSocket.set_option(websocket::stream_base::decorator(
        [](websocket::response_type& response)
        {
            response.set(
                boost::beast::http::field::server,
                "BattleGridServer"
            );
        }
    ));

    auto self = shared_from_this();
    webSocket.async_accept(
        [self](const boost::system::error_code& error)
        {
            self->OnAccept(error);
        }
    );
}

void Session::OnAccept(const boost::system::error_code& error)
{
    if (error)
    {
        LogError("handshake", error);
        return;
    }

    Logger::Info(
        "WebSocket session connected: " + remoteEndpoint
        + " player_id=" + std::to_string(playerId)
    );
    Read();
}

void Session::Read()
{
    auto self = shared_from_this();
    webSocket.async_read(
        buffer,
        [self](const boost::system::error_code& error, std::size_t bytesTransferred)
        {
            self->OnRead(error, bytesTransferred);
        }
    );
}

void Session::OnRead(
    const boost::system::error_code& error,
    std::size_t bytesTransferred
)
{
    static_cast<void>(bytesTransferred);

    if (error == websocket::error::closed)
    {
        Logger::Info("WebSocket session closed: " + remoteEndpoint);
        return;
    }

    if (error)
    {
        LogError("read", error);
        return;
    }

    const std::string message = beast::buffers_to_string(buffer.data());
    Logger::Info("Received raw message from " + remoteEndpoint + ": " + message);

    outboundMessage = dispatcher.Dispatch(message);
    buffer.consume(buffer.size());

    webSocket.text(true);

    auto self = shared_from_this();
    webSocket.async_write(
        boost::asio::buffer(outboundMessage),
        [self](const boost::system::error_code& writeError, std::size_t writtenBytes)
        {
            self->OnWrite(writeError, writtenBytes);
        }
    );
}

void Session::OnWrite(
    const boost::system::error_code& error,
    std::size_t bytesTransferred
)
{
    static_cast<void>(bytesTransferred);

    if (error == websocket::error::closed)
    {
        Logger::Info("WebSocket session closed: " + remoteEndpoint);
        return;
    }

    if (error)
    {
        LogError("write", error);
        return;
    }

    outboundMessage.clear();
    Read();
}

void Session::LogError(
    std::string_view operation,
    const boost::system::error_code& error
) const
{
    Logger::Error(
        "WebSocket session " + std::string(operation) + " error for "
        + remoteEndpoint + ": " + error.message()
    );
}
}
