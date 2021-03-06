#pragma once

#include <iostream>
#include <memory>
#include <algorithm>
#include <mutex>

#include <hoytech/protected_queue.h>
#include <hoytech/time.h>
#include <hoytech/timer.h>
#include <hoytech/hex.h>
#include <uWebSockets/src/uWS.h>
#include <tao/json.hpp>

#include "ethers-cpp/SolidityAbi.h"


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
        uint64_t creation = hoytech::curr_time_us();
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
            if (currWs) terminateCurrentConnection();
            currWs = ws;

            if (onConnect) onConnect();
        });

        hubGroup->onDisconnection([this](uWS::WebSocket<uWS::CLIENT> *ws, int code, char *message, size_t length) {
            std::cout << "Websocket disconnection" << std::endl;
            if (currWs == ws) clearCurrentConnection();
        });

        hubGroup->onError([](void *conn_id) {
            std::cout << "Websocket connection error" << std::endl;
        });

        hubGroup->onMessage2([this](uWS::WebSocket<uWS::CLIENT> *ws, char *message, size_t length, uWS::OpCode opCode, size_t compressedSize) {
            //std::cout << "Compression in: " << compressedSize << " -> " << length << std::endl;

            std::string_view msgStr(message, length);

            try {
                auto msg = tao::json::from_string(msgStr);
                //std::cout << "RECV: " << ": " << tao::json::to_string(msg) << std::endl;

                handleMessage(msg);
            } catch (std::exception &e) {
                std::cerr << "Handle message failure: " << e.what() << ". received: " << msgStr << std::endl;
                terminateCurrentConnection();
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

    tao::json::value sendBatchSync(const tao::json::value &batch) {
        return sendSync("", batch);
    }

    tao::json::value sendSync(const std::string &method, const tao::json::value &params) {
        std::mutex m;
        std::unique_lock<std::mutex> lock(m);
        std::condition_variable cv;

        bool done = false;
        tao::json::value result;

        send(RpcQueryMsg{
            std::move(method),
            std::move(params),
            [&](const tao::json::value &r){
                done = true;
                result = r;
                std::lock_guard<std::mutex> lock(m);
                cv.notify_one();
            },
            [&](const tao::json::value &r){
                done = true;
                result = r;
                std::lock_guard<std::mutex> lock(m);
                cv.notify_one();
            }
        });

        cv.wait(lock, [&]{return done;});

        return result;
    }

    tao::json::value ethCallSync(const std::string &to, EthersCpp::SolidityAbi &abi, const std::string &func, const tao::json::value &data) {
        std::string encodedData = abi.encodeFunctionData(func, data);

        auto r = sendSync("eth_call", tao::json::value::array({
            {
                { "to", to },
                { "data", hoytech::to_hex(encodedData, true) },
            },
            "latest"
        }));

        if (r.is_object()) return r;

        return { { "result", abi.decodeFunctionResult(func, hoytech::from_hex(r.get_string())) } };
    }

    void trigger() {
        hubTrigger->send();
    }

    bool isConnected() {
        return !!currWs;
    }


  private:

    void terminateCurrentConnection() {
        currWs->terminate();
        clearCurrentConnection();
    }

    void clearCurrentConnection() {
        tao::json::value err = { { "error", "reset" } };

        for (const auto &[key, value] : rpcQueryLookup) value.errCb(err);
        rpcQueryLookup.clear();

        for (const auto &[key, value] : rpcSubscriptionLookup) value.errCb(err);
        rpcSubscriptionLookup.clear();

        auto tempQueue = rpcQueryQueue.pop_all_no_wait();
        for (const auto &value : tempQueue) value.errCb(err);

        currWs = nullptr;
    }

    void onAsync() {
        //std::cout << "onAsync" << std::endl;

        if (!currWs) {
            hub.connect(url, nullptr, { }, 5000, hubGroup);
            return;
        }

        for (const auto &[key, value] : rpcQueryLookup) {
            auto now = hoytech::curr_time_us();

            if (value.creation + 60 * 1'000'000UL < now) {
                std::cerr << "Query timeout, reseting connection..." << std::endl;
                terminateCurrentConnection();
                return;
            }
        }

        auto tempQueue = rpcQueryQueue.pop_all_no_wait();

        for (auto &msg : tempQueue) {
            uint64_t queryId = nextRpcQueryId++;

            tao::json::value jsonMsg;

            if (msg.method.size() == 0 && msg.params.is_array()) {
                // batch method
                jsonMsg = msg.params;

                for (auto &e : jsonMsg.get_array()) {
                    e["id"] = queryId;
                    e["jsonrpc"] = "2.0";
                }
            } else {
                jsonMsg = {
                    { "method", msg.method },
                    { "params", msg.params},
                    { "id", queryId },
                    { "jsonrpc", "2.0" },
                };
            }

            std::string encoded = tao::json::to_string(jsonMsg);

            rpcQueryLookup.emplace(queryId, std::move(msg));

            //std::cout << "SENDING: " << queryId << ": " << encoded << std::endl;
            size_t compressedSize;
            currWs->send(encoded.data(), encoded.size(), uWS::OpCode::TEXT, nullptr, nullptr, true, &compressedSize);
            //std::cout << "Compression out: " << encoded.size() << " -> " << compressedSize << std::endl;
        }
    }

    void handleMessage(tao::json::value &msg) {
        if (msg.is_array() || msg.find("id")) {
            uint64_t rpcId;

            if (msg.is_array()) {
                rpcId = msg.get_array().at(0).at("id").get_unsigned();
            } else {
                rpcId = msg.at("id").get_unsigned();
            }

            auto it = rpcQueryLookup.find(rpcId);
            if (it == rpcQueryLookup.end()) {
                std::cerr << "Got response to unknown RPC ID: " << rpcId << std::endl;
                return;
            }

            auto rpcMsg = std::move(it->second);
            rpcQueryLookup.erase(rpcId);

            if (!msg.is_array() && msg.find("error")) {
                std::cerr << "Got RPC error response (" << rpcId << "): " << msg << std::endl;
                rpcMsg.errCb(msg);
                return;
            }

            //std::cerr << "RECV (" << rpcId << "): " << tao::json::to_string(msg) << std::endl;

            if (rpcMsg.method == "eth_subscribe") {
                rpcSubscriptionLookup.emplace(hoytech::from_hex(msg.at("result").get_string()), std::move(rpcMsg));
            } else if (msg.is_array()) {
                tao::json::value res = tao::json::empty_array;

                for (auto &e : msg.get_array()) {
                    res.push_back(e.at("result"));
                }

                rpcMsg.cb(res);
            } else {
                rpcMsg.cb(msg.at("result"));
            }
        } else if (msg.find("method") && msg.at("method").get_string() == "eth_subscription") {
            std::string subsId = hoytech::from_hex(msg.at("params").at("subscription").get_string());

            auto it = rpcSubscriptionLookup.find(subsId);
            if (it == rpcSubscriptionLookup.end()) {
                std::cerr << "Got response to unknown subscription ID: " << subsId << std::endl;
                return;
            }

            auto &rpcMsg = it->second;

            rpcMsg.cb(msg.at("params").at("result"));
        } else {
            throw hoytech::error("Unexpected JSON-RPC message");
        }
    }
};

}
