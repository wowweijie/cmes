#include "ws/client.h"
#include "ws/ftx_client.h"
#include <external/json.hpp>
#include <iostream>

using json = nlohmann::json;


int main()
{
    ws::IWSClient* client;
    client = new ftx::WSClient();
    client->subscribe_ticker("BTC-PERP");

    client->on_message([](json j) { std::cout << "msg: " << j << "\n"; });

    client->connect();
}