#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#include <tao/json.hpp>

#include "hoytech/hex.h"
#include "hoytech/error.h"
#include "ethers-cpp/keccak.h"
#include "ethers-cpp/SolidityAbi.h"
#include "ethers-cpp/ecrecover.h"


int main(int argc, char **argv) {
    std::string abiStr;

    {
        std::ifstream input("artifacts/TestContract.abi");
        std::stringstream sstr;
        while(input >> sstr.rdbuf());
        abiStr = sstr.str();
    }

    EthersCpp::SolidityAbi abi(abiStr);

    if (argc < 2) throw hoytech::error("invalid usage");

    std::string cmd(argv[1]);

    if (cmd == "encodeFunctionData") {
        std::string funcName(argv[2]);
        std::string inputJson(argv[3]);
        auto input = tao::json::from_string(inputJson);
        auto result = abi.encodeFunctionData(funcName, input);
        std::cout << hoytech::to_hex(result, true) << std::endl;
    } else if (cmd == "decodeFunctionResult") {
        std::string funcName(argv[2]);
        std::string result = hoytech::from_hex(argv[3]);
        auto decodedResult = abi.decodeFunctionResult(funcName, result);
        std::cout << tao::json::to_string(decodedResult) << std::endl;
    } else if (cmd == "decodeEvent") {
        std::string topics = hoytech::from_hex(argv[2]);
        std::string data = hoytech::from_hex(argv[3]);
        auto result = abi.decodeEvent(topics, data);
        std::cout << tao::json::to_string(result) << std::endl;
    } else {
        throw hoytech::error("unknown cmd: ", cmd);
    }

    return 0;
}
