#pragma once

#include <iostream>
#include <memory>
#include <algorithm>
#include <mutex>

#include <hoytech/protected_queue.h>
#include <hoytech/timer.h>
#include <uWebSockets/src/uWS.h>
#include <tao/json.hpp>



namespace EthersCpp {

class RpcConnection {
  public:
    using Callback = std::function<void()>;
    using RpcQueryCallback = std::function<void(const tao::json::value &)>;

    struct RpcQueryMsg {
        std::string method;
        tao::json::value params;
        RpcQueryCallback cb;
        RpcQueryCallback errCb = [](const tao::json::value &){};
    };



    uWS::Hub &hub;
    std::string url;

    uWS::Group<uWS::CLIENT> *hubGroup;
    std::unique_ptr<uS::Async> hubTrigger;

    uWS::WebSocket<uWS::CLIENT> *currWs = nullptr;
    Callback onConnect;

    uint64_t nextRpcQueryId = 1;
    std::unordered_map<uint64_t, RpcQueryMsg> rpcQueryLookup;
    std::unordered_map<std::string, RpcQueryMsg> rpcSubscriptionLookup;
    hoytech::protected_queue<RpcQueryMsg> rpcQueryQueue;



    RpcConnection(uWS::Hub &hub_, std::string url_) : hub(hub_), url(url_) {
        hubGroup = hub.createGroup<uWS::CLIENT>(uWS::PERMESSAGE_DEFLATE);

        hubGroup->onConnection([this](uWS::WebSocket<uWS::CLIENT> *ws, uWS::HttpRequest req) {
            if (currWs) {
                currWs->terminate();
                clearCurrentConnection();
            }

            currWs = ws;

            if (onConnect) onConnect();
        });

        hubGroup->onDisconnection([this](uWS::WebSocket<uWS::CLIENT> *ws, int code, char *message, size_t length) {
            std::cout << "Websocket disconnection" << std::endl;
            if (currWs == ws) {
                clearCurrentConnection();
            }
        });

        hubGroup->onError([](void *conn_id) {
            std::cout << "Websocket connection error" << std::endl;
        });

        hubGroup->onMessage2([this](uWS::WebSocket<uWS::CLIENT> *ws, char *message, size_t length, uWS::OpCode opCode, size_t compressedSize) {
            std::string_view msgStr(message, length);

            try {
                auto msg = tao::json::from_string(msgStr);
                //std::cout << "RECV: " << ": " << tao::json::to_string(msg) << std::endl;

                handleMessage(msg);
            } catch (std::exception &e) {
                std::cerr << "Handle message failure: " << e.what() << ". received: " << msgStr << std::endl;
            }
        });

        hubTrigger = std::make_unique<uS::Async>(hub.getLoop());
        hubTrigger->setData(this);

        hubTrigger->start([](uS::Async *a){
            auto *c = static_cast<RpcConnection *>(a->data);
            c->onAsync();
        });

        hubTrigger->send();
    }

    ~RpcConnection() {
        if (currWs) currWs->terminate();
        // FIXME: HubGroup is leaked
    }

    void send(RpcQueryMsg &&msg) {
        rpcQueryQueue.push_move(msg);
        hubTrigger->send();
    }

    void trigger() {
        hubTrigger->send();
    }

    bool isConnected() {
        return !!currWs;
    }


  private:

    void clearCurrentConnection() {
        rpcQueryLookup.clear();
        rpcSubscriptionLookup.clear();
        currWs = nullptr;
    }

    void onAsync() {
        //std::cout << "onAsync" << std::endl;

        if (!currWs) {
            hub.connect(url, nullptr, { }, 5000, hubGroup);
            return;
        }

        auto tempQueue = rpcQueryQueue.pop_all_no_wait();

        for (auto &msg : tempQueue) {
            uint64_t queryId = nextRpcQueryId++;

            tao::json::value jsonMsg = {
                { "method", msg.method },
                { "params", msg.params},
                { "id", queryId },
                { "jsonrpc", "2.0" },
            };

            std::string encoded = tao::json::to_string(jsonMsg);

            rpcQueryLookup.emplace(queryId, std::move(msg));

            //std::cout << "SENDING: " << queryId << ": " << encoded << std::endl;
            size_t compressedSize;
            currWs->send(encoded.data(), encoded.size(), uWS::OpCode::TEXT, nullptr, nullptr, true, &compressedSize);
            //std::cout << "Compression: " << encoded.size() << " -> " << compressedSize << std::endl;
        }
    }

    void handleMessage(tao::json::value &msg) {
        if (msg.find("id")) {
            uint64_t rpcId = msg.at("id").get_unsigned();

            auto it = rpcQueryLookup.find(rpcId);
            if (it == rpcQueryLookup.end()) {
                std::cerr << "Got response to unknown RPC ID: " << rpcId << std::endl;
                return;
            }

            auto rpcMsg = std::move(it->second);
            rpcQueryLookup.erase(rpcId);

            if (msg.find("error")) {
                std::cerr << "Got RPC error response (" << rpcId << "): " << msg << std::endl;
                rpcMsg.errCb(msg);
                return;
            }

            //std::cerr << "RECV (" << rpcId << "): " << tao::json::to_string(msg.at("result")) << std::endl;

            if (rpcMsg.method == "eth_subscribe") {
                rpcSubscriptionLookup.emplace(from_hex(msg.at("result").get_string()), std::move(rpcMsg));
            } else {
                rpcMsg.cb(msg.at("result"));
            }
        } else if (msg.find("method") && msg.at("method").get_string() == "eth_subscription") {
            std::string subsId = from_hex(msg.at("params").at("subscription").get_string());

            auto it = rpcSubscriptionLookup.find(subsId);
            if (it == rpcSubscriptionLookup.end()) {
                std::cerr << "Got response to unknown subscription ID: " << subsId << std::endl;
                return;
            }

            auto &rpcMsg = it->second;

            rpcMsg.cb(msg.at("params").at("result"));
        } else {
            std::cerr << "Unexpected JSON-RPC message: " << tao::json::to_string(msg) << std::endl;
        }
    }
};

}
