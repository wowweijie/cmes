#ifndef INCLUDE_CCAPI_CPP_SERVICE_CCAPI_MARKET_DATA_SERVICE_BITFINEX_H_
#define INCLUDE_CCAPI_CPP_SERVICE_CCAPI_MARKET_DATA_SERVICE_BITFINEX_H_
#ifdef CCAPI_ENABLE_SERVICE_MARKET_DATA
#ifdef CCAPI_ENABLE_EXCHANGE_BITFINEX
#include "ccapi_cpp/service/ccapi_market_data_service.h"
namespace ccapi {
class MarketDataServiceBitfinex : public MarketDataService {
 public:
  MarketDataServiceBitfinex(std::function<void(Event& event)> wsEventHandler, SessionOptions sessionOptions, SessionConfigs sessionConfigs,
                            std::shared_ptr<ServiceContext> serviceContextPtr)
      : MarketDataService(wsEventHandler, sessionOptions, sessionConfigs, serviceContextPtr) {
    this->exchangeName = CCAPI_EXCHANGE_NAME_BITFINEX;
    this->baseUrl = sessionConfigs.getUrlWebsocketBase().at(this->exchangeName);
    this->baseUrlRest = sessionConfigs.getUrlRestBase().at(this->exchangeName);
    this->setHostRestFromUrlRest(this->baseUrlRest);
    try {
      this->tcpResolverResultsRest = this->resolver.resolve(this->hostRest, this->portRest);
    } catch (const std::exception& e) {
      CCAPI_LOGGER_FATAL(std::string("e.what() = ") + e.what());
    }
    this->getRecentTradesTarget = "/v2/trades/{Symbol}/hist";
    this->getInstrumentTarget = "/v2/conf/pub:list:pair:exchange";
    // this->convertNumberToStringInJsonRegex = std::regex("([,\\[:])(-?\\d+\\.?\\d*[eE]?-?\\d*)");
  }
  virtual ~MarketDataServiceBitfinex() {}
#ifndef CCAPI_EXPOSE_INTERNAL

 private:
#endif
  void prepareSubscriptionDetail(std::string& channelId, const std::string& field, const WsConnection& wsConnection, const std::string& symbolId,
                                 const std::map<std::string, std::string> optionMap) override {
    auto marketDepthRequested = std::stoi(optionMap.at(CCAPI_MARKET_DEPTH_MAX));
    CCAPI_LOGGER_TRACE("marketDepthRequested = " + toString(marketDepthRequested));
    auto conflateIntervalMilliSeconds = std::stoi(optionMap.at(CCAPI_CONFLATE_INTERVAL_MILLISECONDS));
    CCAPI_LOGGER_TRACE("conflateIntervalMilliSeconds = " + toString(conflateIntervalMilliSeconds));
    if (field == CCAPI_MARKET_DEPTH) {
      int marketDepthSubscribedToExchange = 1;
      marketDepthSubscribedToExchange = this->calculateMarketDepthSubscribedToExchange(marketDepthRequested, std::vector<int>({1, 25, 100, 250}));
      channelId += std::string("?") + CCAPI_MARKET_DEPTH_SUBSCRIBED_TO_EXCHANGE + "=" + std::to_string(marketDepthSubscribedToExchange);
      this->marketDepthSubscribedToExchangeByConnectionIdChannelIdSymbolIdMap[wsConnection.id][channelId][symbolId] = marketDepthSubscribedToExchange;
    }
  }
  std::vector<std::string> createSendStringList(const WsConnection& wsConnection) override { return std::vector<std::string>(); }
  void onOpen(wspp::connection_hdl hdl) override {
    CCAPI_LOGGER_FUNCTION_ENTER;
    MarketDataService::onOpen(hdl);
    rj::Document document;
    document.SetObject();
    rj::Document::AllocatorType& allocator = document.GetAllocator();
    document.AddMember("event", rj::Value("conf").Move(), allocator);
    document.AddMember("flags", rj::Value(229376).Move(), allocator);
    rj::StringBuffer stringBuffer;
    rj::Writer<rj::StringBuffer> writer(stringBuffer);
    document.Accept(writer);
    std::string sendString = stringBuffer.GetString();
    CCAPI_LOGGER_INFO("sendString = " + toString(sendString));
    ErrorCode ec;
    this->send(hdl, sendString, wspp::frame::opcode::text, ec);
    if (ec) {
      this->onError(Event::Type::SUBSCRIPTION_STATUS, Message::Type::SUBSCRIPTION_FAILURE, ec, "subscribe");
    }
    CCAPI_LOGGER_FUNCTION_EXIT;
  }
  void onClose(wspp::connection_hdl hdl) override {
    CCAPI_LOGGER_FUNCTION_ENTER;
    WsConnection& wsConnection = this->getWsConnectionFromConnectionPtr(this->serviceContextPtr->tlsClientPtr->get_con_from_hdl(hdl));
    this->marketDataMessageDataBufferByConnectionIdExchangeSubscriptionIdMap.erase(wsConnection.id);
    this->sequenceByConnectionIdMap.erase(wsConnection.id);
    MarketDataService::onClose(hdl);
    CCAPI_LOGGER_FUNCTION_EXIT;
  }
  void processTextMessage(WsConnection& wsConnection, wspp::connection_hdl hdl, const std::string& textMessage, const TimePoint& timeReceived, Event& event,
                          std::vector<MarketDataMessage>& marketDataMessageList) override {
    CCAPI_LOGGER_FUNCTION_ENTER;
    rj::Document document;
    // std::string quotedTextMessage = this->convertNumberToStringInJson(textMessage);
    // CCAPI_LOGGER_TRACE("quotedTextMessage = " + quotedTextMessage);
    document.Parse<rj::kParseNumbersAsStringsFlag>(textMessage.c_str());
    CCAPI_LOGGER_TRACE("document.IsObject() = " + toString(document.IsObject()));
    if (document.IsArray() && document.Size() >= 1) {
      auto exchangeSubscriptionId = std::string(document[0].GetString());
      CCAPI_LOGGER_TRACE("exchangeSubscriptionId = " + exchangeSubscriptionId);
      if (document.Size() >= 2 && document[1].IsString() && std::string(document[1].GetString()) == "hb") {
        CCAPI_LOGGER_DEBUG("heartbeat: " + toString(wsConnection));
        if (this->sessionOptions.enableCheckSequence) {
          int sequence = std::stoi(document[2].GetString());
          if (!this->checkSequence(wsConnection, sequence)) {
            this->onOutOfSequence(wsConnection, sequence, hdl, textMessage, timeReceived, exchangeSubscriptionId);
            return;
          }
        }
      } else {
        auto channelId = this->channelIdSymbolIdByConnectionIdExchangeSubscriptionIdMap.at(wsConnection.id).at(exchangeSubscriptionId).at(CCAPI_CHANNEL_ID);
        CCAPI_LOGGER_TRACE("channelId = " + channelId);
        auto symbolId = this->channelIdSymbolIdByConnectionIdExchangeSubscriptionIdMap.at(wsConnection.id).at(exchangeSubscriptionId).at(CCAPI_SYMBOL_ID);
        CCAPI_LOGGER_TRACE("symbolId = " + symbolId);
        if (channelId.rfind(CCAPI_WEBSOCKET_BITFINEX_CHANNEL_BOOK, 0) == 0 || channelId == CCAPI_WEBSOCKET_BITFINEX_CHANNEL_TRADES) {
          std::string channel =
              channelId.rfind(CCAPI_WEBSOCKET_BITFINEX_CHANNEL_BOOK, 0) == 0 ? CCAPI_WEBSOCKET_BITFINEX_CHANNEL_BOOK : CCAPI_WEBSOCKET_BITFINEX_CHANNEL_TRADES;
          rj::Value data(rj::kArrayType);
          if (document[1].IsArray()) {
            if (this->sessionOptions.enableCheckSequence) {
              int sequence = std::stoi(document[2].GetString());
              if (!this->checkSequence(wsConnection, sequence)) {
                this->onOutOfSequence(wsConnection, sequence, hdl, textMessage, timeReceived, exchangeSubscriptionId);
                return;
              }
            }
            const rj::Value& content = document[1].GetArray();
            if (content[0].IsArray()) {
              CCAPI_LOGGER_TRACE("SOLICITED");
              if (this->sessionOptions.enableCheckSequence) {
                int sequence = std::stoi(document[2].GetString());
                CCAPI_LOGGER_INFO("snapshot sequence is " + toString(sequence));
              }
              if (channelId.rfind(CCAPI_WEBSOCKET_BITFINEX_CHANNEL_BOOK, 0) == 0) {
                MarketDataMessage marketDataMessage;
                marketDataMessage.type = MarketDataMessage::Type::MARKET_DATA_EVENTS_MARKET_DEPTH;
                marketDataMessage.exchangeSubscriptionId = exchangeSubscriptionId;
                marketDataMessage.recapType = MarketDataMessage::RecapType::SOLICITED;
                marketDataMessage.tp = timeReceived;
                for (const auto& x : content.GetArray()) {
                  MarketDataMessage::TypeForDataPoint dataPoint;
                  dataPoint.insert({MarketDataMessage::DataFieldType::PRICE, UtilString::normalizeDecimalString(x[0].GetString())});
                  auto count = std::string(x[1].GetString());
                  auto amount = UtilString::normalizeDecimalString(x[2].GetString());
                  if (count != "0") {
                    if (amount.at(0) == '-') {
                      dataPoint.insert({MarketDataMessage::DataFieldType::SIZE, amount.substr(1)});
                      marketDataMessage.data[MarketDataMessage::DataType::ASK].push_back(std::move(dataPoint));
                    } else {
                      dataPoint.insert({MarketDataMessage::DataFieldType::SIZE, amount});
                      marketDataMessage.data[MarketDataMessage::DataType::BID].push_back(std::move(dataPoint));
                    }
                  } else {
                    if (amount.at(0) == '-') {
                      dataPoint.insert({MarketDataMessage::DataFieldType::SIZE, "0"});
                      marketDataMessage.data[MarketDataMessage::DataType::ASK].push_back(std::move(dataPoint));
                    } else {
                      dataPoint.insert({MarketDataMessage::DataFieldType::SIZE, "0"});
                      marketDataMessage.data[MarketDataMessage::DataType::BID].push_back(std::move(dataPoint));
                    }
                  }
                }
                marketDataMessageList.push_back(std::move(marketDataMessage));
              } else {
                for (const auto& x : content.GetArray()) {
                  MarketDataMessage marketDataMessage;
                  marketDataMessage.type = MarketDataMessage::Type::MARKET_DATA_EVENTS_TRADE;
                  marketDataMessage.exchangeSubscriptionId = exchangeSubscriptionId;
                  marketDataMessage.recapType = MarketDataMessage::RecapType::SOLICITED;
                  std::string timestampInMilliseconds = x[1].GetString();
                  timestampInMilliseconds.insert(timestampInMilliseconds.size() - 3, ".");
                  marketDataMessage.tp = UtilTime::makeTimePoint(UtilTime::divide(timestampInMilliseconds));
                  MarketDataMessage::TypeForDataPoint dataPoint;
                  dataPoint.insert({MarketDataMessage::DataFieldType::PRICE, UtilString::normalizeDecimalString(std::string(x[3].GetString()))});
                  auto amount = UtilString::normalizeDecimalString(x[2].GetString());
                  dataPoint.insert({MarketDataMessage::DataFieldType::SIZE, amount.at(0) == '-' ? amount.substr(1) : amount});
                  dataPoint.insert({MarketDataMessage::DataFieldType::TRADE_ID, std::string(x[0].GetString())});
                  dataPoint.insert({MarketDataMessage::DataFieldType::IS_BUYER_MAKER, amount.at(0) == '-' ? "1" : "0"});
                  marketDataMessage.data[MarketDataMessage::DataType::TRADE].push_back(std::move(dataPoint));
                  marketDataMessageList.push_back(std::move(marketDataMessage));
                }
              }
            } else {
              const rj::Value& x = content;
              MarketDataMessage::TypeForDataPoint dataPoint;
              dataPoint.insert({MarketDataMessage::DataFieldType::PRICE, UtilString::normalizeDecimalString(x[0].GetString())});
              auto count = std::string(x[1].GetString());
              auto amount = UtilString::normalizeDecimalString(x[2].GetString());
              if (count != "0") {
                if (amount.at(0) == '-') {
                  dataPoint.insert({MarketDataMessage::DataFieldType::SIZE, amount.substr(1)});
                  this->marketDataMessageDataBufferByConnectionIdExchangeSubscriptionIdMap[wsConnection.id][exchangeSubscriptionId]
                                                                                          [MarketDataMessage::DataType::ASK]
                                                                                              .push_back(std::move(dataPoint));
                } else {
                  dataPoint.insert({MarketDataMessage::DataFieldType::SIZE, amount});
                  this->marketDataMessageDataBufferByConnectionIdExchangeSubscriptionIdMap[wsConnection.id][exchangeSubscriptionId]
                                                                                          [MarketDataMessage::DataType::BID]
                                                                                              .push_back(std::move(dataPoint));
                }
              } else {
                if (amount.at(0) == '-') {
                  dataPoint.insert({MarketDataMessage::DataFieldType::SIZE, "0"});
                  this->marketDataMessageDataBufferByConnectionIdExchangeSubscriptionIdMap[wsConnection.id][exchangeSubscriptionId]
                                                                                          [MarketDataMessage::DataType::ASK]
                                                                                              .push_back(std::move(dataPoint));
                } else {
                  dataPoint.insert({MarketDataMessage::DataFieldType::SIZE, "0"});
                  this->marketDataMessageDataBufferByConnectionIdExchangeSubscriptionIdMap[wsConnection.id][exchangeSubscriptionId]
                                                                                          [MarketDataMessage::DataType::BID]
                                                                                              .push_back(std::move(dataPoint));
                }
              }
            }
          } else if (document[1].IsString()) {
            std::string str = document[1].GetString();
            if (str == "tu" || str == "te") {
              if (this->sessionOptions.enableCheckSequence) {
                int sequence = std::stoi(document[3].GetString());
                if (!this->checkSequence(wsConnection, sequence)) {
                  this->onOutOfSequence(wsConnection, sequence, hdl, textMessage, timeReceived, exchangeSubscriptionId);
                  return;
                }
              }
              if (str == CCAPI_BITFINEX_STREAM_TRADE_RAW_MESSAGE_TYPE) {
                const rj::Value& x = document[2].GetArray();
                MarketDataMessage marketDataMessage;
                marketDataMessage.type = MarketDataMessage::Type::MARKET_DATA_EVENTS_TRADE;
                marketDataMessage.exchangeSubscriptionId = exchangeSubscriptionId;
                marketDataMessage.recapType = MarketDataMessage::RecapType::NONE;
                std::string timestampInMilliseconds = x[1].GetString();
                timestampInMilliseconds.insert(timestampInMilliseconds.size() - 3, ".");
                marketDataMessage.tp = UtilTime::makeTimePoint(UtilTime::divide(timestampInMilliseconds));
                MarketDataMessage::TypeForDataPoint dataPoint;
                dataPoint.insert({MarketDataMessage::DataFieldType::PRICE, UtilString::normalizeDecimalString(std::string(x[3].GetString()))});
                auto amount = UtilString::normalizeDecimalString(x[2].GetString());
                dataPoint.insert({MarketDataMessage::DataFieldType::SIZE, amount.at(0) == '-' ? amount.substr(1) : amount});
                if (str == "tu") {
                  dataPoint.insert({MarketDataMessage::DataFieldType::TRADE_ID, std::string(x[0].GetString())});
                }
                dataPoint.insert({MarketDataMessage::DataFieldType::IS_BUYER_MAKER, amount.at(0) == '-' ? "1" : "0"});
                marketDataMessage.data[MarketDataMessage::DataType::TRADE].push_back(std::move(dataPoint));
                marketDataMessageList.push_back(std::move(marketDataMessage));
              }
            } else if (str == "cs") {
              if (this->sessionOptions.enableCheckSequence) {
                int sequence = std::stoi(document[3].GetString());
                if (!this->checkSequence(wsConnection, sequence)) {
                  this->onOutOfSequence(wsConnection, sequence, hdl, textMessage, timeReceived, exchangeSubscriptionId);
                  return;
                }
              }
              if (this->sessionOptions.enableCheckOrderBookChecksum) {
                this->orderBookChecksumByConnectionIdSymbolIdMap[wsConnection.id][symbolId] =
                    intToHex(static_cast<uint_fast32_t>(static_cast<uint32_t>(std::stoi(document[2].GetString()))));
              }
              MarketDataMessage marketDataMessage;
              marketDataMessage.type = MarketDataMessage::Type::MARKET_DATA_EVENTS_MARKET_DEPTH;
              std::string timestampInMilliseconds = document[4].GetString();
              timestampInMilliseconds.insert(timestampInMilliseconds.size() - 3, ".");
              marketDataMessage.tp = UtilTime::makeTimePoint(UtilTime::divide(timestampInMilliseconds));
              marketDataMessage.exchangeSubscriptionId = exchangeSubscriptionId;
              CCAPI_LOGGER_TRACE("NONE");
              marketDataMessage.recapType = MarketDataMessage::RecapType::NONE;
              std::swap(marketDataMessage.data,
                        this->marketDataMessageDataBufferByConnectionIdExchangeSubscriptionIdMap[wsConnection.id][exchangeSubscriptionId]);
              marketDataMessageList.push_back(std::move(marketDataMessage));
            }
          }
        }
      }
    } else if (document.IsObject() && document.HasMember("event")) {
      std::string eventStr = document["event"].GetString();
      if (eventStr == "conf") {
        std::vector<std::string> sendStringList;
        CCAPI_LOGGER_TRACE("this->subscriptionListByConnectionIdChannelSymbolIdMap = " + toString(this->subscriptionListByConnectionIdChannelIdSymbolIdMap));
        for (const auto& subscriptionListByChannelIdSymbolId : this->subscriptionListByConnectionIdChannelIdSymbolIdMap.at(wsConnection.id)) {
          auto channelId = subscriptionListByChannelIdSymbolId.first;
          if (channelId.rfind(CCAPI_WEBSOCKET_BITFINEX_CHANNEL_BOOK, 0) == 0 || channelId == CCAPI_WEBSOCKET_BITFINEX_CHANNEL_TRADES) {
            std::string channel = channelId.rfind(CCAPI_WEBSOCKET_BITFINEX_CHANNEL_BOOK, 0) == 0 ? CCAPI_WEBSOCKET_BITFINEX_CHANNEL_BOOK
                                                                                                 : CCAPI_WEBSOCKET_BITFINEX_CHANNEL_TRADES;
            for (auto& subscriptionListBySymbolId : subscriptionListByChannelIdSymbolId.second) {
              rj::Document document;
              document.SetObject();
              rj::Document::AllocatorType& allocator = document.GetAllocator();
              document.AddMember("event", rj::Value("subscribe").Move(), allocator);
              document.AddMember("channel", rj::Value(channel.c_str(), allocator).Move(), allocator);
              auto symbolId = subscriptionListBySymbolId.first;
              document.AddMember("symbol", rj::Value(symbolId.c_str(), allocator).Move(), allocator);
              if (channel == CCAPI_WEBSOCKET_BITFINEX_CHANNEL_BOOK) {
                document.AddMember("prec", rj::Value("P0").Move(), allocator);
                document.AddMember("freq", rj::Value("F0").Move(), allocator);
                document.AddMember(
                    "len",
                    rj::Value(
                        std::to_string(this->marketDepthSubscribedToExchangeByConnectionIdChannelIdSymbolIdMap.at(wsConnection.id).at(channelId).at(symbolId))
                            .c_str(),
                        allocator)
                        .Move(),
                    allocator);
              }
              rj::StringBuffer stringBuffer;
              rj::Writer<rj::StringBuffer> writer(stringBuffer);
              document.Accept(writer);
              std::string sendString = stringBuffer.GetString();
              sendStringList.push_back(std::move(sendString));
            }
          }
        }
        for (const auto& sendString : sendStringList) {
          CCAPI_LOGGER_INFO("sendString = " + sendString);
          ErrorCode ec;
          this->send(hdl, sendString, wspp::frame::opcode::text, ec);
          if (ec) {
            this->onError(Event::Type::SUBSCRIPTION_STATUS, Message::Type::SUBSCRIPTION_FAILURE, ec, "subscribe");
          }
        }
      } else if (eventStr == "subscribed" || eventStr == "error") {
        std::string channel = document["channel"].GetString();
        auto channelId = channel == CCAPI_WEBSOCKET_BITFINEX_CHANNEL_BOOK
                             ? channel + "?" + CCAPI_MARKET_DEPTH_SUBSCRIBED_TO_EXCHANGE + "=" + std::string(document["len"].GetString())
                             : channel;
        CCAPI_LOGGER_TRACE("channelId = " + channelId);
        auto symbolId = std::string(document["symbol"].GetString());
        CCAPI_LOGGER_TRACE("symbolId = " + symbolId);
        if (eventStr == "subscribed") {
          auto exchangeSubscriptionId = std::string(document["chanId"].GetString());
          CCAPI_LOGGER_TRACE("exchangeSubscriptionId = " + exchangeSubscriptionId);
          this->channelIdSymbolIdByConnectionIdExchangeSubscriptionIdMap[wsConnection.id][exchangeSubscriptionId][CCAPI_CHANNEL_ID] = channelId;
          this->channelIdSymbolIdByConnectionIdExchangeSubscriptionIdMap[wsConnection.id][exchangeSubscriptionId][CCAPI_SYMBOL_ID] = symbolId;
          CCAPI_LOGGER_TRACE("this->channelIdSymbolIdByConnectionIdExchangeSubscriptionIdMap = " +
                             toString(this->channelIdSymbolIdByConnectionIdExchangeSubscriptionIdMap));
        }
        event.setType(Event::Type::SUBSCRIPTION_STATUS);
        std::vector<Message> messageList;
        Message message;
        message.setTimeReceived(timeReceived);
        std::vector<std::string> correlationIdList;
        if (this->correlationIdListByConnectionIdChannelIdSymbolIdMap.find(wsConnection.id) !=
            this->correlationIdListByConnectionIdChannelIdSymbolIdMap.end()) {
          if (this->correlationIdListByConnectionIdChannelIdSymbolIdMap.at(wsConnection.id).find(channelId) !=
              this->correlationIdListByConnectionIdChannelIdSymbolIdMap.at(wsConnection.id).end()) {
            if (this->correlationIdListByConnectionIdChannelIdSymbolIdMap.at(wsConnection.id).at(channelId).find(symbolId) !=
                this->correlationIdListByConnectionIdChannelIdSymbolIdMap.at(wsConnection.id).at(channelId).end()) {
              std::vector<std::string> correlationIdList_2 =
                  this->correlationIdListByConnectionIdChannelIdSymbolIdMap.at(wsConnection.id).at(channelId).at(symbolId);
              correlationIdList.insert(correlationIdList.end(), correlationIdList_2.begin(), correlationIdList_2.end());
            }
          }
        }
        message.setCorrelationIdList(correlationIdList);
        message.setType(eventStr == "subscribed" ? Message::Type::SUBSCRIPTION_STARTED : Message::Type::SUBSCRIPTION_FAILURE);
        Element element;
        element.insert(eventStr == "subscribed" ? CCAPI_INFO_MESSAGE : CCAPI_ERROR_MESSAGE, textMessage);
        message.setElementList({element});
        messageList.push_back(std::move(message));
        event.setMessageList(messageList);
      }
    }
    CCAPI_LOGGER_FUNCTION_EXIT;
  }
  bool checkSequence(const WsConnection& wsConnection, int sequence) {
    if (this->sequenceByConnectionIdMap.find(wsConnection.id) == this->sequenceByConnectionIdMap.end()) {
      if (sequence != this->sessionConfigs.getInitialSequenceByExchangeMap().at(this->exchangeName)) {
        CCAPI_LOGGER_WARN("incorrect initial sequence, wsConnection = " + toString(wsConnection));
        return false;
      }
      this->sequenceByConnectionIdMap.insert(std::pair<std::string, int>(wsConnection.id, sequence));
      return true;
    } else {
      CCAPI_LOGGER_DEBUG("sequence: previous = " + toString(this->sequenceByConnectionIdMap[wsConnection.id]) + ", current = " + toString(sequence) +
                         ", wsConnection = " + toString(wsConnection));
      if (sequence - this->sequenceByConnectionIdMap[wsConnection.id] == 1) {
        this->sequenceByConnectionIdMap[wsConnection.id] = sequence;
        return true;
      } else {
        return false;
      }
    }
  }
  void onOutOfSequence(WsConnection& wsConnection, int sequence, wspp::connection_hdl hdl, const std::string& textMessage, const TimePoint& timeReceived,
                       const std::string& exchangeSubscriptionId) {
    int previous = 0;
    if (this->sequenceByConnectionIdMap.find(wsConnection.id) != this->sequenceByConnectionIdMap.end()) {
      previous = this->sequenceByConnectionIdMap[wsConnection.id];
    }
    this->onError(Event::Type::SUBSCRIPTION_STATUS, Message::Type::INCORRECT_STATE_FOUND,
                  "out of sequence: previous = " + toString(previous) + ", current = " + toString(sequence) + ", connection = " + toString(wsConnection) +
                      ", textMessage = " + textMessage + ", timeReceived = " + UtilTime::getISOTimestamp(timeReceived));
    this->marketDataMessageDataBufferByConnectionIdExchangeSubscriptionIdMap[wsConnection.id][exchangeSubscriptionId].clear();
    ErrorCode ec;
    this->close(wsConnection, hdl, websocketpp::close::status::normal, "out of sequence", ec);
    if (ec) {
      this->onError(Event::Type::SUBSCRIPTION_STATUS, Message::Type::GENERIC_ERROR, ec, "shutdown");
    }
    this->shouldProcessRemainingMessageOnClosingByConnectionIdMap[wsConnection.id] = false;
  }
  std::string calculateOrderBookChecksum(const std::map<Decimal, std::string>& snapshotBid, const std::map<Decimal, std::string>& snapshotAsk) override {
    auto i = 0;
    auto i1 = snapshotBid.rbegin();
    auto i2 = snapshotAsk.begin();
    std::vector<std::string> csData;
    while (i < 25 && (i1 != snapshotBid.rend() || i2 != snapshotAsk.end())) {
      if (i1 != snapshotBid.rend()) {
        csData.push_back(i1->first.toString());
        csData.push_back(i1->second);
        ++i1;
      }
      if (i2 != snapshotAsk.end()) {
        csData.push_back(i2->first.toString());
        csData.push_back("-" + i2->second);
        ++i2;
      }
      ++i;
    }
    std::string csStr = UtilString::join(csData, ":");
    CCAPI_LOGGER_DEBUG("csStr = " + csStr);
    uint_fast32_t csCalc = UtilAlgorithm::crc(csStr.begin(), csStr.end());
    return intToHex(csCalc);
  }
  void onIncorrectStatesFound(WsConnection& wsConnection, wspp::connection_hdl hdl, const std::string& textMessage, const TimePoint& timeReceived,
                              const std::string& exchangeSubscriptionId, std::string const& reason) override {
    CCAPI_LOGGER_ERROR("incorrect states found: connection = " + toString(wsConnection) + ", textMessage = " + textMessage +
                       ", timeReceived = " + UtilTime::getISOTimestamp(timeReceived) + ", reason = " + reason);
    this->marketDataMessageDataBufferByConnectionIdExchangeSubscriptionIdMap[wsConnection.id][exchangeSubscriptionId].clear();
    MarketDataService::onIncorrectStatesFound(wsConnection, hdl, textMessage, timeReceived, exchangeSubscriptionId, reason);
  }
  void convertRequestForRest(http::request<http::string_body>& req, const Request& request, const TimePoint& now, const std::string& symbolId,
                             const std::map<std::string, std::string>& credential) override {
    switch (request.getOperation()) {
      case Request::Operation::GENERIC_PUBLIC_REQUEST: {
        MarketDataService::convertRequestForRestGenericPublicRequest(req, request, now, symbolId, credential);
      } break;
      case Request::Operation::GET_RECENT_TRADES: {
        req.method(http::verb::get);
        auto target = this->getRecentTradesTarget;
        this->substituteParam(target, {
                                          {"{Symbol}", symbolId},
                                      });
        std::string queryString;
        const std::map<std::string, std::string> param = request.getFirstParamWithDefault();
        this->appendParam(queryString, param,
                          {
                              {CCAPI_LIMIT, "limit"},
                          });
        req.target(target + "?" + queryString);
      } break;
      case Request::Operation::GET_INSTRUMENT: {
        req.method(http::verb::get);
        auto target = this->getInstrumentTarget;
        req.target(target);
      } break;
      default:
        this->convertRequestForRestCustom(req, request, now, symbolId, credential);
    }
  }
  // void processSuccessfulTextMessageRest(int statusCode, const Request& request, const std::string& textMessage, const TimePoint& timeReceived) override {
  //   std::string quotedTextMessage = this->convertNumberToStringInJson(textMessage);
  //   CCAPI_LOGGER_TRACE("quotedTextMessage = " + quotedTextMessage);
  //   MarketDataService::processSuccessfulTextMessageRest(statusCode, request, quotedTextMessage, timeReceived);
  // }
  void convertTextMessageToMarketDataMessage(const Request& request, const std::string& textMessage, const TimePoint& timeReceived, Event& event,
                                             std::vector<MarketDataMessage>& marketDataMessageList) override {
    rj::Document document;
    document.Parse<rj::kParseNumbersAsStringsFlag>(textMessage.c_str());
    switch (request.getOperation()) {
      case Request::Operation::GET_RECENT_TRADES: {
        for (const auto& x : document.GetArray()) {
          MarketDataMessage marketDataMessage;
          marketDataMessage.type = MarketDataMessage::Type::MARKET_DATA_EVENTS_TRADE;
          marketDataMessage.tp = UtilTime::makeTimePointFromMilliseconds(std::stoll(x[1].GetString()));
          MarketDataMessage::TypeForDataPoint dataPoint;
          std::string rawAmount = UtilString::normalizeDecimalString(x[2].GetString());
          bool isAmountNegative = !rawAmount.empty() && rawAmount.at(0) == '-';
          dataPoint.insert({MarketDataMessage::DataFieldType::PRICE, UtilString::normalizeDecimalString(std::string(x[3].GetString()))});
          dataPoint.insert({MarketDataMessage::DataFieldType::SIZE, isAmountNegative ? rawAmount.substr(1) : rawAmount});
          dataPoint.insert({MarketDataMessage::DataFieldType::TRADE_ID, std::string(x[0].GetString())});
          dataPoint.insert({MarketDataMessage::DataFieldType::IS_BUYER_MAKER, isAmountNegative ? "1" : "0"});
          marketDataMessage.data[MarketDataMessage::DataType::TRADE].push_back(std::move(dataPoint));
          marketDataMessageList.push_back(std::move(marketDataMessage));
        }
      } break;
      case Request::Operation::GET_INSTRUMENT: {
        Message message;
        message.setTimeReceived(timeReceived);
        message.setType(this->requestOperationToMessageTypeMap.at(request.getOperation()));
        for (const auto& x : document[0].GetArray()) {
          std::string pair = x.GetString();
          if (pair == request.getInstrument().substr(1)) {
            Element element;
            auto splitted = UtilString::split(pair, ":");
            if (splitted.size() == 1) {
              element.insert(CCAPI_BASE_ASSET, pair.substr(0, 3));
              element.insert(CCAPI_QUOTE_ASSET, pair.substr(3, 6));
            } else {
              element.insert(CCAPI_BASE_ASSET, splitted.at(0));
              element.insert(CCAPI_QUOTE_ASSET, splitted.at(1));
            }
            element.insert(CCAPI_ORDER_PRICE_INCREMENT, "0." + std::string(8 - 1, '0') + "1");
            element.insert(CCAPI_ORDER_QUANTITY_INCREMENT, "0." + std::string(8 - 1, '0') + "1");
            message.setElementList({element});
            break;
          }
        }
        message.setCorrelationIdList({request.getCorrelationId()});
        event.addMessages({message});
      } break;
      default:
        CCAPI_LOGGER_FATAL(CCAPI_UNSUPPORTED_VALUE);
    }
  }
  std::map<std::string, std::map<std::string, MarketDataMessage::TypeForData> > marketDataMessageDataBufferByConnectionIdExchangeSubscriptionIdMap;
  std::map<std::string, int> sequenceByConnectionIdMap;
};
} /* namespace ccapi */
#endif
#endif
#endif  // INCLUDE_CCAPI_CPP_SERVICE_CCAPI_MARKET_DATA_SERVICE_BITFINEX_H_
