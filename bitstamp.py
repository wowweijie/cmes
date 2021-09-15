# -*- coding: utf-8 -*-
import websocket
import json
import ssl

bitstamp_endpoint = 'wss://ws.bitstamp.net'


def subscribe_marketdata(ws):
    params = {
        'event': 'bts:subscribe',
        'data': {
            'channel': 'order_book_btcusd'
        }
    }
    market_depth_subscription = json.dumps(params)

    ws.send(market_depth_subscription)


def on_open(ws):
    print('web-socket connected.')
    subscribe_marketdata(ws)


def on_message(ws, data):
    data = json.loads(data)
    print(data)


def on_error(ws, msg):
    print(msg)


if __name__ == '__main__':
    marketdata_ws = websocket.WebSocketApp(
        bitstamp_endpoint, on_open=on_open, on_message=on_message, on_error=on_error)
    marketdata_ws.run_forever(sslopt={'cert_reqs': ssl.CERT_NONE})
