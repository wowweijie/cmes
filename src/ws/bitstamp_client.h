#pragma once

#include "util/WS.h"
#include <external/json.hpp>
#include <string>
#include <vector>

using json = nlohmann::json;

namespace bitstamp{

class WSClient
{
  public:
    WSClient();

    void on_message(util::WS::OnMessageCB cb);
    void connect();
    std::vector<json> on_open();

    void subscribe_ticker(std::string currency_pair);
    void subscribe_orders(std::string currency_pair);
    void subscribe_orderbook(std::string currency_pair);
    void subscribe_detail_orderbook(std::string currency_pair);
    void subscribe_full_orderbook(std::string currency_pair);

  private:
    std::vector<std::string> subscriptions;
    util::WS::OnMessageCB message_cb;
    util::WS ws;
    const std::string uri = "wss://ws.bitstamp.net";
    // const std::string api_key = "";
    // const std::string api_secret = "";
    // const std::string subaccount_name = "";
};

}
