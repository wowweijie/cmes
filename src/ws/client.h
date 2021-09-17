#pragma once

#include "util/WS.h"
#include <external/json.hpp>
#include <string>
#include <vector>

using json = nlohmann::json;

namespace ws {

    class IWSClient
    {
    public:
        IWSClient() {};

        virtual void on_message(util::WS::OnMessageCB cb) = 0;
        virtual void connect() = 0;
        virtual std::vector<json> on_open() = 0;

        virtual void subscribe_orders(std::string market) = 0;
        virtual void subscribe_orderbook(std::string market) = 0;
        virtual void subscribe_fills(std::string market) = 0;
        virtual void subscribe_trades(std::string market) = 0;
        virtual void subscribe_ticker(std::string market) = 0;
    };
}
