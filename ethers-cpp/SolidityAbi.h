#pragma once

#include <unordered_map>
#include <string>
#include <string_view>
#include <vector>
#include <queue>
#include <memory>

#include <gmpxx.h>

#include <tao/json.hpp>
#include "hoytech/error.h"
#include "hoytech/hex.h"

#include "ethers-cpp/keccak.h"


namespace EthersCpp {


class SolidityAbi {
  public:
    SolidityAbi(std::string_view abi) {
        auto json = tao::json::from_string(abi);
        _init(json);
    }

    SolidityAbi(tao::json::value &abi) {
        _init(abi);
    }

    std::string encodeFunctionData(std::string_view funcName, tao::json::value &input) {
        auto it = functions.find(std::string(funcName));
        if (it == functions.end()) throw hoytech::error("unable to encode unknown solidity abi function: ", funcName);
        auto &function = it->second;

        return function.sigHash + _abiEncode(function.item.at("inputs").get_array(), input);
    }

    tao::json::value decodeFunctionResult(std::string_view funcName, std::string_view result) {
        auto it = functions.find(std::string(funcName));
        if (it == functions.end()) throw hoytech::error("unable to decode unknown solidity abi function: ", funcName);
        auto &function = it->second;

        return _abiDecode(function.item.at("outputs").get_array(), result);
    }

    tao::json::value decodeEvent(std::string_view topics, std::string_view data) {
        auto it = events.find(std::string(topics.substr(0, 32)));
        if (it == events.end()) throw hoytech::error("unable to decode solidity abi event");
        auto &event = it->second;

        tao::json::value output = _abiDecode(event.indexedItems, topics.substr(32));
        tao::json::value output2 = _abiDecode(event.nonIndexedItems, data);

        for (auto &pair : output2.get_object()) output[pair.first] = pair.second;

        return { { "name", event.name }, { "args", output } };
    }



  private:
    struct Event {
        std::string name;
        std::vector<tao::json::value> indexedItems;
        std::vector<tao::json::value> nonIndexedItems;
    };

    std::unordered_map<std::string, Event> events;


    struct Function {
        tao::json::value item;
        std::string sigHash;
    };

    std::unordered_map<std::string, Function> functions;


    void _init(tao::json::value &abi) {
        for (auto &item : abi.get_array()) {
            auto name = item.at("name").get_string();
            auto type = item.at("type").get_string();
            auto format = _getFormat(name, item.at("inputs"));
            auto formatHash = keccak256(format);

            if (type == "event") {
                Event e;

                e.name = name;

                for (auto &input : item.at("inputs").get_array()) {
                    if (input.at("indexed").get_boolean()) e.indexedItems.push_back(input);
                    else e.nonIndexedItems.push_back(input);
                }

                events.emplace(formatHash, std::move(e));
            } else if (type == "function") {
                Function f;

                f.item = item;
                f.sigHash = formatHash.substr(0, 4);

                if (functions.find(name) != functions.end()) {
                    std::cerr << "WARNING: Duplicate solidity function name: " << name << std::endl;
                    continue;
                }

                functions.emplace(name, std::move(f));
            }
        }
    }

    std::string _getFormat(std::string &name, tao::json::value &inputs) {
        auto output = name;

        output += "(";

        bool first = true;

        for (auto &input : inputs.get_array()) {
            if (!first) output += ",";
            first = false;

            auto type = input.at("type").get_string();

            if (type.substr(0, 5) == "tuple") {
                std::string empty;
                output += _getFormat(empty, input.at("components"));
                output += type.substr(5); // any following array modifiers
            } else {
                output += type;
            }
        }

        output += ")";

        return output;
    }

    struct ParseNode {
        std::string fieldName;
        std::string base;
        int byteWidth = -1;
        bool dynamic = false;
        std::unique_ptr<ParseNode> arrayOf;
        long fixedArraySize = -1;
        std::vector<ParseNode> components;

        ParseNode(std::vector<tao::json::value> &items) {
            tao::json::value root({ { "name", "" }, { "components", items } });
            _init(root, "tuple");
            dynamic = false; // root tuple doesn't need indirection
        }

        // Interior node constructor
        ParseNode(tao::json::value &field, std::string type) {
            _init(field, type);
        }

      private:
        void _init(tao::json::value &field, std::string type) {
            fieldName = field.at("name").get_string();

            if (type.ends_with("]")) {
                auto pos = type.find_last_of('[');
                if (pos == std::string::npos) throw hoytech::error("unbalanced array brackets in type: ", type);
                auto arrayLenSpec = type.substr(pos + 1, type.size() - pos - 2);

                if (arrayLenSpec == "") dynamic = true;
                else fixedArraySize = stol(arrayLenSpec);

                arrayOf = make_unique<ParseNode>(field, type.substr(0, pos));
                if (arrayOf->dynamic) dynamic = true;

                return;
            }

            std::size_t curr = type.find_first_not_of("abcdefghijklmnopqrstuvwxyz");
            if (curr == std::string::npos) curr = type.size();
            base = type.substr(0, curr);
            type = type.substr(curr);

            curr = type.find_first_not_of("0123456789");
            if (curr == std::string::npos) curr = type.size();
            if (curr > 0) {
                byteWidth = stoi(type.substr(0, curr));
                if (base == "uint" || base == "int") byteWidth /= 8;
                type = type.substr(curr);
            }

            if (base == "tuple") {
                for (auto &c : field.at("components").get_array()) {
                    components.push_back(ParseNode(c, c.at("type").get_string()));
                    if (components.back().dynamic) dynamic = true;
                }
            } else if (base == "string" || (base == "bytes" && byteWidth == -1)) {
                dynamic = true;
            }
        }
    };


    tao::json::value _abiDecode(std::vector<tao::json::value> &items, std::string_view buffer) {
        size_t currOffset = 0;

        auto consume = [&](size_t n = 32){
            auto slice = buffer.substr(currOffset, n);
            if (slice.size() != n) throw hoytech::error("buffer underrun");
            currOffset += n;
            return slice;
        };

        auto followPointer = [&](std::function<void()> cb){
            auto ptr = convertToMpz(consume()).get_ui();
            auto savedOffset = currOffset;
            currOffset = ptr;

            cb();

            currOffset = savedOffset;
        };

        auto newOffsetBasis = [&](std::function<void()> cb){
            auto savedBuffer = buffer;
            buffer = buffer.substr(currOffset);
            currOffset = 0;

            cb();

            buffer = savedBuffer;
        };

        std::function<tao::json::value(ParseNode &)> process = [&](ParseNode &node) -> tao::json::value {
            if (node.arrayOf) {
                tao::json::value arr = tao::json::empty_array;

                if (node.dynamic) {
                    followPointer([&]{
                        size_t len = node.fixedArraySize > -1 ? node.fixedArraySize : convertToMpz(consume()).get_ui();
                        newOffsetBasis([&]{
                            for (size_t i = 0; i < len; i++) {
                                arr.push_back(process(*node.arrayOf));
                            }
                        });
                    });
                } else {
                    for (auto i = 0; i < node.fixedArraySize; i++) {
                        arr.push_back(process(*node.arrayOf));
                    }
                }

                return arr;
            } else if (node.base == "tuple") {
                tao::json::value o = tao::json::empty_object;

                if (node.dynamic) {
                    followPointer([&]{
                        newOffsetBasis([&]{
                            for (auto &component : node.components) {
                                o[component.fieldName] = process(component);
                            }
                        });
                    });
                } else {
                    for (auto &component : node.components) {
                        o[component.fieldName] = process(component);
                    }
                }

                return o;
            } else if (node.base == "address") {
                return hoytech::to_hex(consume().substr(12), true);
            } else if (node.base == "uint") {
                return convertToMpz(consume()).get_str();
            } else if (node.base == "int") {
                return convertToMpzSigned(consume()).get_str();
            } else if (node.base == "bool") {
                return convertToMpz(consume()) != 0;
            } else if (node.base == "string") {
                std::string str;

                followPointer([&]{
                    size_t len = convertToMpz(consume()).get_ui();
                    str = consume(len);
                });

                return str;
            } else if (node.base == "bytes" && node.byteWidth == -1) {
                std::string str;

                followPointer([&]{
                    size_t len = convertToMpz(consume()).get_ui();
                    str = consume(len);
                });

                return hoytech::to_hex(str, true);
            } else if (node.base == "bytes") {
                auto v = consume();
                return hoytech::to_hex(v.substr(0, node.byteWidth), true);
            } else {
                throw hoytech::error("unrecognized type: ", node.base);
            }
        };

        ParseNode tree(items);

        return process(tree);
    }

    std::string _abiEncode(std::vector<tao::json::value> &rootItems, tao::json::value &input) {
        std::string output;
        std::queue<std::function<void()>> pending;

        auto append = [&](std::string_view data){
            output += data;

            size_t partialSize = data.size() % 32;

            if (partialSize) {
                size_t paddingSize = 32 - partialSize;
                output += std::string(paddingSize, '\0');
            }
        };

        std::function<void(ParseNode &, tao::json::value &, uint64_t)> process = [&](ParseNode &node, tao::json::value &item, uint64_t offset){
            if (node.dynamic) {
                auto slotLocation = output.size();
                append("X"); // slot for pointer, will be padded to 32 bytes

                pending.push([&, offset, slotLocation]{
                    output.replace(slotLocation, 32, normaliseUnsigned(output.size() - offset));

                    if (node.arrayOf) {
                        if (node.fixedArraySize == -1) append(normaliseUnsigned(item.get_array().size()));
                        uint64_t newOffset = output.size();
                        for (auto &i : item.get_array()) {
                            process(*node.arrayOf, i, newOffset);
                        }
                    } else if (node.base == "tuple") {
                        uint64_t newOffset = output.size();
                        for (auto &component : node.components) {
                            process(component, item.at(component.fieldName), newOffset);
                        }
                    } else if (node.base == "string") {
                        auto &str = item.get_string();
                        append(normaliseUnsigned(str.size()));
                        append(str);
                    } else if (node.base == "bytes" && node.byteWidth == -1) {
                        auto str = hoytech::from_hex(item.get_string());
                        append(normaliseUnsigned(str.size()));
                        append(str);
                    } else {
                        throw hoytech::error("unexpected type: ", node.base);
                    }
                });
            } else {
                if (node.arrayOf) {
                    for (auto &i : item.get_array()) {
                        process(*node.arrayOf, i, offset);
                    }
                } else if (node.base == "tuple") {
                    for (auto &component : node.components) {
                        process(component, item.at(component.fieldName), offset);
                    }
                } else if (node.base == "address") {
                    auto str = item.get_string();
                    if (str.size() != 42) throw hoytech::error("bad length for address: ", item.get_string());
                    append(normaliseHexStr(str));
                } else if (node.base == "bytes" && node.byteWidth > -1) {
                    auto str = hoytech::from_hex(item.get_string());
                    if (str.size() != static_cast<size_t>(node.byteWidth)) throw hoytech::error("bad length for bytesN: ", item.get_string());
                    append(str);
                } else if (node.base == "bool") {
                    append(normaliseUnsigned(item.get_boolean() ? 1 : 0));
                } else if (node.base == "uint") {
                    if (item.is_integer()) {
                        uint64_t v = item.get_unsigned();
                        append(normaliseUnsigned(v));
                    } else {
                        mpz_class n(item.get_string());
                        if (n < 0) throw hoytech::error("value for uint is negative: ", item.get_string());
                        append(normaliseMpz(n));
                    }
                } else if (node.base == "int") {
                    if (item.is_unsigned()) {
                        uint64_t v = item.get_unsigned();
                        append(normaliseUnsigned(v));
                    } else if (item.is_signed()) {
                        int64_t v = item.get_signed();
                        append(normaliseSigned(v));
                    } else {
                        mpz_class n(item.get_string());
                        append(normaliseSignedMpz(n));
                    }
                } else {
                    throw hoytech::error("unexpected type: ", node.base);
                }
            }
        };

        ParseNode tree(rootItems);
        process(tree, input, 0);

        while (!pending.empty()) {
            auto cb = pending.front();
            pending.pop();
            cb();
        }

        return output;
    }




    std::string normaliseHexStr(std::string_view hexStr, size_t numBytes = 32) {
        std::string str(hexStr);
        if (str.substr(0, 2) == "0x") str = str.substr(2);
        str.replace(0, 0, std::string(numBytes*2 - str.length(), '0'));
        return hoytech::from_hex(str);
    }

    std::string normaliseMpz(mpz_class &num, size_t numBytes = 32) {
        auto str = num.get_str(16);
        if (str.length() > numBytes*2) throw hoytech::error("input exceeds numBytes");
        return normaliseHexStr(str, numBytes);
    }

    std::string normaliseUnsigned(uint64_t num, size_t numBytes = 32) {
        mpz_class numMpz{num};
        return normaliseMpz(numMpz, numBytes);
    }

    mpz_class twoToThe256 = mpz_class("115792089237316195423570985008687907853269984665640564039457584007913129639936");

    std::string normaliseSignedMpz(mpz_class &numMpz, size_t numBytes = 32) {
        if (numMpz < 0) numMpz += twoToThe256;

        return normaliseMpz(numMpz, numBytes);
    }

    std::string normaliseSigned(int64_t num, size_t numBytes = 32) {
        mpz_class numMpz{num};

        if (numMpz < 0) numMpz += twoToThe256;

        return normaliseMpz(numMpz, numBytes);
    }

    mpz_class convertToMpz(std::string_view str) {
        if (str.size() == 0) return mpz_class(0);
        std::string s = hoytech::to_hex(str);
        return mpz_class{s, 16};
    }

    mpz_class convertToMpzSigned(std::string_view str) {
        auto num = convertToMpz(str);

        if (mpz_tstbit(num.get_mpz_t(), 255)) {
            return -(twoToThe256 - num);
        }

        return num;
    }
};


}
