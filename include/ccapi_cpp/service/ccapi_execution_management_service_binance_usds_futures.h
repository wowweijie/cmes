#ifndef INCLUDE_CCAPI_CPP_SERVICE_CCAPI_EXECUTION_MANAGEMENT_SERVICE_BINANCE_USDS_FUTURES_H_
#define INCLUDE_CCAPI_CPP_SERVICE_CCAPI_EXECUTION_MANAGEMENT_SERVICE_BINANCE_USDS_FUTURES_H_
#ifdef CCAPI_ENABLE_SERVICE_EXECUTION_MANAGEMENT
#ifdef CCAPI_ENABLE_EXCHANGE_BINANCE_USDS_FUTURES
#include "ccapi_cpp/service/ccapi_execution_management_service_binance_derivatives_base.h"
namespace ccapi {
class ExecutionManagementServiceBinanceUsdsFutures : public ExecutionManagementServiceBinanceDerivativesBase {
 public:
  ExecutionManagementServiceBinanceUsdsFutures(std::function<void(Event& event)> eventHandler, SessionOptions sessionOptions, SessionConfigs sessionConfigs,
                                               ServiceContextPtr serviceContextPtr)
      : ExecutionManagementServiceBinanceDerivativesBase(eventHandler, sessionOptions, sessionConfigs, serviceContextPtr) {
    CCAPI_LOGGER_FUNCTION_ENTER;
    this->exchangeName = CCAPI_EXCHANGE_NAME_BINANCE_USDS_FUTURES;
    this->baseUrl = sessionConfigs.getUrlWebsocketBase().at(this->exchangeName) + "/ws";
    this->baseUrlRest = sessionConfigs.getUrlRestBase().at(this->exchangeName);
    this->setHostRestFromUrlRest(this->baseUrlRest);
    try {
      this->tcpResolverResultsRest = this->resolver.resolve(this->hostRest, this->portRest);
    } catch (const std::exception& e) {
      CCAPI_LOGGER_FATAL(std::string("e.what() = ") + e.what());
    }
    this->apiKeyName = CCAPI_BINANCE_USDS_FUTURES_API_KEY;
    this->apiSecretName = CCAPI_BINANCE_USDS_FUTURES_API_SECRET;
    this->setupCredential({this->apiKeyName, this->apiSecretName});
    this->createOrderTarget = CCAPI_BINANCE_USDS_FUTURES_CREATE_ORDER_PATH;
    this->cancelOrderTarget = "/fapi/v1/order";
    this->getOrderTarget = "/fapi/v1/order";
    this->getOpenOrdersTarget = "/fapi/v1/openOrders";
    this->cancelOpenOrdersTarget = "/fapi/v1/allOpenOrders";
    this->isDerivatives = true;
    this->listenKeyTarget = CCAPI_BINANCE_USDS_FUTURES_LISTEN_KEY_PATH;
    this->getAccountBalancesTarget = "/fapi/v2/account";
    this->getAccountPositionsTarget = "/fapi/v2/account";
    CCAPI_LOGGER_FUNCTION_EXIT;
  }
  virtual ~ExecutionManagementServiceBinanceUsdsFutures() {}
};
} /* namespace ccapi */
#endif
#endif
#endif  // INCLUDE_CCAPI_CPP_SERVICE_CCAPI_EXECUTION_MANAGEMENT_SERVICE_BINANCE_USDS_FUTURES_H_
