#include <memory>
#include <oatpp-websocket/ConnectionHandler.hpp>
#include <oatpp-websocket/Handshaker.hpp>
#include <oatpp-websocket/WebSocket.hpp>
#include <oatpp/core/macro/component.hpp>
#include <oatpp/network/Server.hpp>
#include <oatpp/network/tcp/server/ConnectionProvider.hpp>
#include <oatpp/parser/json/mapping/ObjectMapper.hpp>
#include <oatpp/web/server/HttpConnectionHandler.hpp>
#include <oatpp/web/server/HttpRouter.hpp>

#include "oatpp_controller.h"
#include "oatpp_ws_listener.h"

// Based on Oatpp examples
// https://github.com/oatpp/example-websocket

struct AppComponent {
    OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::network::ServerConnectionProvider>,
                           serverConnectionProvider)
    ([] {
        return oatpp::network::tcp::server::ConnectionProvider::createShared(
            {"0.0.0.0", 8000, oatpp::network::Address::IP_4});
    }());

    // Router component
    OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::web::server::HttpRouter>, httpRouter)
    ([] { return oatpp::web::server::HttpRouter::createShared(); }());

    // HTTP connection handler (is this needed?)
    OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::network::ConnectionHandler>,
                           httpConnectionHandler)
    ("http", [] {
        OATPP_COMPONENT(std::shared_ptr<oatpp::web::server::HttpRouter>,
                        router);  // get Router component
        return oatpp::web::server::HttpConnectionHandler::createShared(router);
    }());

    // Create ObjectMapper component to serialize/deserialize DTOs in Contoller's API
    OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::data::mapping::ObjectMapper>,
                           apiObjectMapper)
    ([] { return oatpp::parser::json::mapping::ObjectMapper::createShared(); }());

    // Websocket connection handler
    OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::network::ConnectionHandler>,
                           websocketConnectionHandler)
    ("websocket", [] {
        auto connectionHandler = oatpp::websocket::ConnectionHandler::createShared();
        connectionHandler->setSocketInstanceListener(
            std::make_shared<OatppWSInstanceListener>());
        return connectionHandler;
    }());
};

void run()
{
    /* Register Components in scope of run() method */
    AppComponent components;

    /* Get router component */
    OATPP_COMPONENT(std::shared_ptr<oatpp::web::server::HttpRouter>, router);

    /* Create OatppController and add all of its endpoints to router */
    router->addController(std::make_shared<OatppController>());

    /* Get connection handler component */
    OATPP_COMPONENT(
        std::shared_ptr<oatpp::network::ConnectionHandler>, connectionHandler, "http");

    /* Get connection provider component */
    OATPP_COMPONENT(std::shared_ptr<oatpp::network::ServerConnectionProvider>,
                    connectionProvider);

    /* Create server which takes provided TCP connections and passes them to HTTP connection
     * handler */
    oatpp::network::Server server(connectionProvider, connectionHandler);

    /* Priny info about server port */
    OATPP_LOGI("MyApp",
               "Server running on port %s",
               connectionProvider->getProperty("port").getData());

    /* Run server */
    server.run();
}

int main(int argc, const char *argv[])
{
    oatpp::base::Environment::init();

    run();

    oatpp::base::Environment::destroy();

    return 0;
}
