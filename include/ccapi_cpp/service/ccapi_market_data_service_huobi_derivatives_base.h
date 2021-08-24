#ifndef INCLUDE_CCAPI_CPP_SERVICE_CCAPI_MARKET_DATA_SERVICE_HUOBI_DERIVATIVES_BASE_H_
#define INCLUDE_CCAPI_CPP_SERVICE_CCAPI_MARKET_DATA_SERVICE_HUOBI_DERIVATIVES_BASE_H_
#ifdef CCAPI_ENABLE_SERVICE_MARKET_DATA
#if defined(CCAPI_ENABLE_EXCHANGE_HUOBI_USDT_SWAP) || defined(CCAPI_ENABLE_EXCHANGE_HUOBI_COIN_SWAP)
#include "ccapi_cpp/service/ccapi_market_data_service_huobi_base.h"
namespace ccapi {
class MarketDataServiceHuobiDerivativesBase : public MarketDataServiceHuobiBase {
 public:
  MarketDataServiceHuobiDerivativesBase(std::function<void(Event& event)> wsEventHandler, SessionOptions sessionOptions, SessionConfigs sessionConfigs,
                                        std::shared_ptr<ServiceContext> serviceContextPtr)
      : MarketDataServiceHuobiBase(wsEventHandler, sessionOptions, sessionConfigs, serviceContextPtr) {
    this->isDerivatives = true;
  }
  virtual ~MarketDataServiceHuobiDerivativesBase() {}
  bool doesHttpBodyContainError(const Request& request, const std::string& body) override { return body.find("err_code") != std::string::npos; }
  void convertRequestForRest(http::request<http::string_body>& req, const Request& request, const TimePoint& now, const std::string& symbolId,
                             const std::map<std::string, std::string>& credential) override {
    switch (request.getOperation()) {
      case Request::Operation::GET_INSTRUMENT: {
        req.method(http::verb::get);
        auto target = this->getInstrumentTarget;
        std::string queryString;
        this->appendSymbolId(queryString, symbolId, "contract_code");
        req.target(target + "?" + queryString);
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
          if (std::string(x["contract_code"].GetString()) == request.getInstrument()) {
            Element element;
            element.insert(CCAPI_ORDER_PRICE_INCREMENT, UtilString::normalizeDecimalString(x["price_tick"].GetString()));
            element.insert(CCAPI_ORDER_QUANTITY_INCREMENT, UtilString::normalizeDecimalString(x["contract_size"].GetString()));
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
#endif  // INCLUDE_CCAPI_CPP_SERVICE_CCAPI_MARKET_DATA_SERVICE_HUOBI_DERIVATIVES_BASE_H_
