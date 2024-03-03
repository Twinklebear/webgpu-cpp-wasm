#include "App.h"
#include "WebSocketProtocol.h"

struct PerSocketData {
    // No data
};

int main(int argc, const char *argv[])
{
    uWS::App()
        .ws<PerSocketData>(
            "/*",
            {// High max payload and idle timeout for the demo/test
             .maxPayloadLength = 1024 * 1024 * 1024,
             .idleTimeout = 600,
             .open = [](auto *ws) { std::cout << "New connection opened\n"; },
             .message =
                 [](auto *ws, std::string_view message, uWS::OpCode op_code) {
                     std::cout << "Receive message of size: " << message.size() << ": "
                               << message << "\n";

                     std::cout << "Message opcode: ";
                     switch (op_code) {
                     case uWS::OpCode::CONTINUATION:
                         std::cout << "CONTINUATION\n";
                         break;
                     case uWS::OpCode::TEXT:
                         std::cout << "TEXT\n";
                         break;
                     case uWS::OpCode::BINARY:
                         std::cout << "BINARY\n";
                         break;
                     case uWS::OpCode::CLOSE:
                         std::cout << "CLOSE\n";
                         break;
                     case uWS::OpCode::PING:
                         std::cout << "PING\n";
                         break;
                     case uWS::OpCode::PONG:
                         std::cout << "PONG\n";
                         break;
                     }

                     // Send back some data to the demo app, text and binary
                     const std::string text_message =
                         "Hello from uWebSocket: " + std::string(message);
                     ws->send(text_message, uWS::OpCode::TEXT);

                     // And some binary data
                     const std::string bin_message =
                         "This is sent as binary: " + std::string(message);
                     ws->send(text_message);
                 },
             .dropped =
                 [](auto * /*ws*/, std::string_view /*message*/, uWS::OpCode /*opCode*/) {
                     /* A message was dropped due to set maxBackpressure and
                      * closeOnBackpressureLimit limit */
                     std::cout << "Dropped\n";
                 },
             .drain =
                 [](auto *ws) {
                     std::cout << "Drain, buffer amount = " << ws->getBufferedAmount() << "\n";
                     /* Check ws->getBufferedAmount() here */
                 },
             .ping =
                 [](auto *ws, std::string_view) {
                     std::cout << "PING\n";
                     ws->send("pong", uWS::OpCode::PONG);
                 },
             .pong = [](auto * /*ws*/,
                        std::string_view view) { std::cout << "Got pong: " << view << "\n"; },
             .close =
                 [](auto * /*ws*/, int code, std::string_view message) {
                     /* You may access ws->getUserData() here */
                     std::cout << "socket closed: " << message << ", code = " << code << "\n";
                 }})
        .listen(8000,
                [](auto *listenSocket) {
                    if (listenSocket) {
                        std::cout << "Listening on 8000\n";
                    }
                })
        .run();

    return 0;
}
