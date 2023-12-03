#include "oatpp_ws_listener.h"
#include <iostream>

void OatppWSListener::onPing(const WebSocket &socket, const oatpp::String &message)
{
    std::cout << __PRETTY_FUNCTION__ << "\n";
    socket.sendPong(message);
}

void OatppWSListener::onPong(const WebSocket &socket, const oatpp::String &message) {}

void OatppWSListener::onClose(const WebSocket &socket,
                              v_uint16 code,
                              const oatpp::String &message)
{
    std::cout << __PRETTY_FUNCTION__ << ": code = " << code
              << ", message: " << message->c_str() << "\n";
}

void OatppWSListener::readMessage(const WebSocket &socket,
                                  v_uint8 opcode,
                                  p_char8 data,
                                  oatpp::v_io_size size)
{
    std::cout << __PRETTY_FUNCTION__ << " size = " << size << "\n";
    if (size == 0) {  // message transfer finished
        auto msg = message_buffer.toString();
        message_buffer.setCurrentPosition(0);

        // TODO: we could get binary data
        std::cout << __PRETTY_FUNCTION__ << ": message = " << msg->c_str() << "\n";

        // Send a test text and binary message as examples to test receiving them
        // on the emscripten side.
        socket.sendOneFrameText("Hello from oatpp!: " + msg);
        socket.sendOneFrameBinary("this is a binary string. Echo: " + msg);

    } else if (size > 0) {
        // Received message data, append to the buffer
        message_buffer.writeSimple(data, size);
    }
}

void OatppWSInstanceListener::onAfterCreate(const oatpp::websocket::WebSocket &socket,
                                            const std::shared_ptr<const ParameterMap> &params)
{
    std::cout << __PRETTY_FUNCTION__ << "\n";

    // TODO: Example makes a listener per websocket, but we don't need this really
    // For the test app we'll only have one websocket listener, and in apps
    // with more connections they could share listeners if possible. E.g. listener
    // could maintain a map of WebSocket -> socket recv data state to handle multiple
    // incoming connections if needed. Or it may be simpler just to have multiple
    socket.setListener(std::make_shared<OatppWSListener>());
}

void OatppWSInstanceListener::onBeforeDestroy(const oatpp::websocket::WebSocket &socket)
{
    std::cout << __PRETTY_FUNCTION__ << "\n";
}
