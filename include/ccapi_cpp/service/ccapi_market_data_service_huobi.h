#ifndef INCLUDE_CCAPI_CPP_SERVICE_CCAPI_MARKET_DATA_SERVICE_HUOBI_H_
#define INCLUDE_CCAPI_CPP_SERVICE_CCAPI_MARKET_DATA_SERVICE_HUOBI_H_
#ifdef CCAPI_ENABLE_SERVICE_MARKET_DATA
#ifdef CCAPI_ENABLE_EXCHANGE_HUOBI
#include "ccapi_cpp/service/ccapi_market_data_service_huobi_base.h"
namespace ccapi {
class MarketDataServiceHuobi : public MarketDataServiceHuobiBase {
 public:
  MarketDataServiceHuobi(std::function<void(Event& event)> wsEventHandler, SessionOptions sessionOptions, SessionConfigs sessionConfigs,
                         std::shared_ptr<ServiceContext> serviceContextPtr)
      : MarketDataServiceHuobiBase(wsEventHandler, sessionOptions, sessionConfigs, serviceContextPtr) {
    this->exchangeName = CCAPI_EXCHANGE_NAME_HUOBI;
    this->baseUrl = sessionConfigs.getUrlWebsocketBase().at(this->exchangeName);
    this->baseUrlRest = sessionConfigs.getUrlRestBase().at(this->exchangeName);
    this->setHostRestFromUrlRest(this->baseUrlRest);
    try {
      this->tcpResolverResultsRest = this->resolver.resolve(this->hostRest, this->portRest);
    } catch (const std::exception& e) {
      CCAPI_LOGGER_FATAL(std::string("e.what() = ") + e.what());
    }
    this->getRecentTradesTarget = "/market/history/trade";
    this->getInstrumentTarget = "/v1/common/symbols";
  }
  virtual ~MarketDataServiceHuobi() {}
  bool doesHttpBodyContainError(const Request& request, const std::string& body) override { return body.find("err-code") != std::string::npos; }
  void convertRequestForRest(http::request<http::string_body>& req, const Request& request, const TimePoint& now, const std::string& symbolId,
                             const std::map<std::string, std::string>& credential) override {
    switch (request.getOperation()) {
      case Request::Operation::GET_INSTRUMENT: {
        req.method(http::verb::get);
        auto target = this->getInstrumentTarget;
        req.target(target);
      } break;
      default:
        MarketDataServiceHuobiBase::convertRequestForRest(req, request, now, symbolId, credential);
    }
  }
  void convertTextMessageToMarketDataMessage(const Request& request, const std::string& textMessage, const TimePoint& timeReceived, Event& event,
                                             std::vector<MarketDataMessage>& marketDataMessageList) override {
    switch (request.getOperation()) {
      case Request::Operation::GET_INSTRUMENT: {
        rj::Document document;
        document.Parse<rj::kParseNumbersAsStringsFlag>(textMessage.c_str());
        Message message;
        message.setTimeReceived(timeReceived);
        message.setType(this->requestOperationToMessageTypeMap.at(request.getOperation()));
        for (const auto& x : document["data"].GetArray()) {
          if (std::string(x["symbol"].GetString()) == request.getInstrument()) {
            Element element;
            element.insert(CCAPI_BASE_ASSET, x["base-currency"].GetString());
            element.insert(CCAPI_QUOTE_ASSET, x["quote-currency"].GetString());
            int pricePrecision = std::stoi(x["price-precision"].GetString());
            element.insert(CCAPI_ORDER_PRICE_INCREMENT, "0." + std::string(pricePrecision - 1, '0') + "1");
            int amountPrecision = std::stoi(x["amount-precision"].GetString());
            element.insert(CCAPI_ORDER_QUANTITY_INCREMENT, "0." + std::string(amountPrecision - 1, '0') + "1");
            message.setElementList({element});
            break;
          }
        }
        message.setCorrelationIdList({request.getCorrelationId()});
        event.addMessages({message});
      } break;
      default:
        MarketDataServiceHuobiBase::convertTextMessageToMarketDataMessage(request, textMessage, timeReceived, event, marketDataMessageList);
    }
  }
};
} /* namespace ccapi */
#endif
#endif
#endif  // INCLUDE_CCAPI_CPP_SERVICE_CCAPI_MARKET_DATA_SERVICE_HUOBI_H_
