#pragma once
#include "encryption.hpp"
#include "sha3_256.hpp"
#include "sha3_512.hpp"
#include "utils.hpp"

// IND-CCA2-secure Key Encapsulation Mechanism
namespace ccakem {

// Given (k * 12 * 32 + 32) -bytes public key, this routine randomly samples 32
// -bytes from system RNG ( random number generator ) and computes cipher text
// of length (k * du * 32 + dv * 32) -bytes which can be shared with recipient
// party ( having respective secret key ) over insecure channel.

// It also returns a SHAKE256 object which acts as a KDF ( key derivation
// function ), used for generating arbitrary length shared secret key, to be
// used for symmetric key encryption between these two participating entities.
//
// Other side of communication should also be able to generate same arbitrary
// length key stream ( using KDF ), after successful decryption of cipher text.
//
// See algorithm 8 defined in Kyber specification, as submitted to NIST PQC
// final round call
// https://csrc.nist.gov/CSRC/media/Projects/post-quantum-cryptography/documents/round-3/submissions/Kyber-Round3.zip
template<const size_t k,
         const size_t eta1,
         const size_t eta2,
         const size_t du,
         const size_t dv>
shake256::shake256
encapsulate(const uint8_t* const __restrict pubkey, // (k * 12 * 32 + 32) -bytes
            uint8_t* const __restrict cipher // (k * du * 32 + dv * 32) -bytes
)
{
  constexpr size_t pklen = k * 12 * 32 + 32;
  constexpr size_t ctlen = k * du * 32 + dv * 32;

  uint8_t m[32]{};
  uint8_t g_in[64]{};
  uint8_t g_out[64]{};
  uint8_t kdf_in[64]{};

  kyber_utils::random_data<uint8_t>(m, sizeof(m));

  sha3_256::hash(m, sizeof(m), g_in);
  sha3_256::hash(pubkey, pklen, g_in + 32);
  sha3_512::hash(g_in, sizeof(g_in), g_out);

  cpapke::encrypt<k, eta1, eta2, du, dv>(pubkey, g_in, g_out + 32, cipher);

  std::memcpy(kdf_in, g_out, 32);
  sha3_256::hash(cipher, ctlen, kdf_in + 32);

  shake256::shake256 hasher{};
  hasher.hash(kdf_in, sizeof(kdf_in));
  return hasher;
}

}