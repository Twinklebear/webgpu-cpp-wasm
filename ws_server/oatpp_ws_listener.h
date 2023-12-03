#pragma once

#include <oatpp-websocket/ConnectionHandler.hpp>
#include <oatpp-websocket/WebSocket.hpp>

// Based on Oatpp examples
// https://github.com/oatpp/example-websocket

// WebSocket listener listens on incoming WebSocket events.
class OatppWSListener : public oatpp::websocket::WebSocket::Listener {
    // Buffer for messages. Needed for multi-frame messages.
    oatpp::data::stream::BufferOutputStream message_buffer;

public:
    // Called on ping frame
    void onPing(const WebSocket &socket, const oatpp::String &message) override;

    // Called on pong frame
    void onPong(const WebSocket &socket, const oatpp::String &message) override;

    // Called on "close" frame when the socket is disconnecting
    void onClose(const WebSocket &socket,
                 v_uint16 code,
                 const oatpp::String &message) override;

    /* Called on each message frame. After the last message will be called once-again with size
     * == 0 to designate end of the message.
     */
    void readMessage(const WebSocket &socket,
                     v_uint8 opcode,
                     p_char8 data,
                     oatpp::v_io_size size) override;
};

// Listener for new WebSocket connections, creates a OatppWSListener for each one
class OatppWSInstanceListener
    : public oatpp::websocket::ConnectionHandler::SocketInstanceListener {
    // Called after creation of a websocket
    void onAfterCreate(const oatpp::websocket::WebSocket &socket,
                       const std::shared_ptr<const ParameterMap> &params) override;

    //  Called before a websocket is destroyed.
    void onBeforeDestroy(const oatpp::websocket::WebSocket &socket) override;
};
