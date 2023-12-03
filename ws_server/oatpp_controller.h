#pragma once

#include <oatpp-websocket/ConnectionHandler.hpp>
#include <oatpp-websocket/Handshaker.hpp>
#include <oatpp-websocket/WebSocket.hpp>
#include <oatpp/core/macro/component.hpp>
#include <oatpp/network/Server.hpp>
#include <oatpp/network/tcp/server/ConnectionProvider.hpp>
#include <oatpp/web/server/HttpConnectionHandler.hpp>
#include <oatpp/web/server/HttpRouter.hpp>

// Based on Oatpp examples
// https://github.com/oatpp/example-websocket

#include OATPP_CODEGEN_BEGIN(ApiController)

// Controller with WebSocket-connect endpoint.
class OatppController : public oatpp::web::server::api::ApiController {
private:
    OATPP_COMPONENT(std::shared_ptr<oatpp::network::ConnectionHandler>,
                    websocketConnectionHandler,
                    "websocket");

public:
    OatppController(OATPP_COMPONENT(std::shared_ptr<ObjectMapper>, objectMapper))
        : oatpp::web::server::api::ApiController(objectMapper)
    {
    }

public:
    ENDPOINT("GET", "/", root)
    {
        return createResponse(Status::CODE_404, "");
    }

    ENDPOINT("GET", "ws", ws, REQUEST(std::shared_ptr<IncomingRequest>, request))
    {
        return oatpp::websocket::Handshaker::serversideHandshake(request->getHeaders(),
                                                                 websocketConnectionHandler);
    };
};

#include OATPP_CODEGEN_END(ApiController)
