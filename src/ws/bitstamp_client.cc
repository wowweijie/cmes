#include "ws/bitstamp_client.h"
#include "util/Encoding.h"
#include "util/Time.h"
#include <utility>

namespace encoding = util::encoding;

namespace bitstamp{

WSClient::WSClient()
{
    ws.configure(uri,"a","b","c");
    ws.set_on_open_cb([this]() { return this->on_open(); });
}

void WSClient::on_message(util::WS::OnMessageCB cb)
{
    ws.set_on_message_cb(cb);
}

void WSClient::connect()
{
    ws.connect();
}

std::vector<json> WSClient::on_open()
{
    std::vector<json> msgs;
    
    for (auto& channel : subscriptions) {
        json msg = {
          {"event", "bts:subscribe"}, 
          {"data",{{"channel",channel}}
          }
          };
        std::cout<<msg<<std::endl;

        msgs.push_back(msg);
    }
    return msgs;
}

void WSClient::subscribe_ticker(std::string currency_pair)
{
    subscriptions.push_back("live_trades_"+currency_pair);
}

void WSClient::subscribe_orders(std::string currency_pair)
{
    subscriptions.push_back("live_orders_"+currency_pair);
}

void WSClient::subscribe_orderbook(std::string currency_pair)
{
    subscriptions.push_back("order_book_"+currency_pair);
}

void WSClient::subscribe_detail_orderbook(std::string currency_pair)
{
    subscriptions.push_back("detail_order_book_"+currency_pair);
}

void WSClient::subscribe_full_orderbook(std::string currency_pair)
{
    subscriptions.push_back("diff_order_book_"+currency_pair);
}

}

