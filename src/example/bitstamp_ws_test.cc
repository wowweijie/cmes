#include "ws/bitstamp_client.h"
#include <external/json.hpp>
#include <iostream>

using json = nlohmann::json;

/*
Currency pairs: 
btcusd, btceur, btcgbp, btcpax, btcusdt,
btcusdc, gbpusd, gbpeur, eurusd, ethusd, etheur, ethbtc, 
ethgbp, ethpax, ethusdt, ethusdc, xrpusd, xrpeur, xrpbtc,
xrpgbp, xrppax, xrpusdt, uniusd, unieur, unibtc, ltcusd, 
ltceur, ltcbtc, ltcgbp, linkusd, linkeur, linkbtc, linkgbp,
linketh, maticusd, maticeur, xlmusd, xlmeur, xlmbtc, xlmgbp,
fttusd, ftteur, bchusd, bcheur, bchbtc, bchgbp, aaveusd, aaveeur,
aavebtc, algousd, algoeur, algobtc, compusd, compeur, compbtc, 
snxusd, snxeur, snxbtc, chzusd, chzeur, enjusd, enjeur, batusd, 
bateur, batbtc, mkrusd, mkreur, mkrbtc, zrxusd, zrxeur, zrxbtc,
yfiusd, yfieur, yfibtc, sushiusd, sushieur, alphausd, alphaeur,
grtusd, grteur, umausd, umaeur, umabtc, omgusd, omgeur, omgbtc,
omggbp, kncusd, knceur, kncbtc, crvusd, crveur, crvbtc, audiousd,
audioeur, audiobtc, storjusd, storjeur, usdtusd, usdteur, usdcusd, 
usdceur, usdcusdt, eurtusd, eurteur, daiusd, paxusd, paxeur, paxgbp, eth2eth, gusdusd

*/
int main()
{
    bitstamp::WSClient client;
    client.subscribe_orders("btcusd");
    client.subscribe_ticker("ethusdt");
    client.subscribe_orderbook("crveur");
    client.subscribe_detail_orderbook("storjusd");
    client.subscribe_full_orderbook("sushiusd");



    client.on_message([](json j) { std::cout << "msg: " << j << "\n"; });

    client.connect();
}
