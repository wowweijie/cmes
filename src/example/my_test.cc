#include "ws/client.h"
#include "ws/ftx_client.h"
#include "engine/engine.h"
#include <external/json.hpp>
#include <iostream>

using json = nlohmann::json;


int main()
{
    std::vector<ws::IWSClient*> gateways;
    gateways.push_back(new ftx::WSClient());
    Engine engine( gateways);
    engine.start();
}