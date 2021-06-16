#pragma once

#include <string_view>

#include <secp256k1.h>
#include <secp256k1_recovery.h>

#include "hoytech/error.h"

#include "ethers-cpp/keccak.h"



namespace EthersCpp {

static secp256k1_context *secp256k1ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);

static inline std::string ecrecover(std::string_view hash, int v, std::string_view r, std::string_view s) {
    std::string signature;
    signature += r;
    signature += s;

    secp256k1_ecdsa_recoverable_signature sig;
    if (!secp256k1_ecdsa_recoverable_signature_parse_compact(secp256k1ctx, &sig, reinterpret_cast<const unsigned char*>(signature.data()), v)) {
        throw hoytech::error("secp256k1_ecdsa_recoverable_signature_parse_compact");
    }

    secp256k1_pubkey pub;
    if (secp256k1_ecdsa_recover(secp256k1ctx, &pub, &sig, reinterpret_cast<const unsigned char*>(hash.data())) == 0) {
        throw hoytech::error("secp256k1_ecdsa_recover");
    }

    char pubKeyBuffer[65];
    size_t pubKeyBufferLength = 65;

    if (!secp256k1_ec_pubkey_serialize(secp256k1ctx, reinterpret_cast<unsigned char*>(&pubKeyBuffer[0]), &pubKeyBufferLength, &pub, SECP256K1_EC_UNCOMPRESSED)) {
        throw hoytech::error("secp256k1_ec_pubkey_serialize");
    }

    if (pubKeyBufferLength != 65) {
        throw hoytech::error("secp256k1_ec_pubkey_serialize: returned bad length: ", pubKeyBufferLength);
    }

    std::string pubKey(&pubKeyBuffer[1], pubKeyBufferLength-1);
    std::string ethAddr = keccak256(pubKey).substr(12);

    return ethAddr;
}

static inline std::string ecrecoverPacked(std::string_view hash, std::string_view r, std::string_view sv) {
    int v = (sv[0] & 0x80) ? 1 : 0;

    std::string s;
    s += sv[0] & 0x7F;
    s += sv.substr(1);

    return ecrecover(hash, v, r, s);
}

}
