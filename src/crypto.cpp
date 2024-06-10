
#include "crypto.hpp"
#include "config.hpp"
#include "rsa_.hpp"
#include <cstring>
#include <fmt/printf.h>
#include <functional>
#include <memory>
#include <openssl/sha.h>
// TODO: this is mocked for now

int leader_for_epoch(int epoch) {
  // TODO: public hash function
  return (epoch % kNodes);
}

void sha256_string(char *string, size_t string_sz, unsigned char *output_hash) {
  // std::cout << __func__ << "sha256 len=" << SHA256_DIGEST_LENGTH << "\n";
  unsigned char hash[SHA256_DIGEST_LENGTH];
  SHA256_CTX sha256;
  SHA256_Init(&sha256);
  SHA256_Update(&sha256, string, strlen(string));
  SHA256_Final(hash, &sha256);
  ::memcpy(output_hash, hash, SHA256_DIGEST_LENGTH);
}

std::unique_ptr<uint8_t[]> compute_hash(uint8_t *serialized_block,
                                        size_t blk_sz) {

  // TODO: compute cryptographic hmac
  auto h = std::make_unique<uint8_t[]>(k_hash_size);
  ::memset(h.get(), '0', k_hash_size);

  sha256_string(reinterpret_cast<char *>(serialized_block), blk_sz,
                reinterpret_cast<unsigned char *>(h.get()));
#if 0
  std::cout << __func__ << "\n";
   for (auto i = 0ULL; i < k_hash_size; i++) {
      std::cout << static_cast<int>(h.get()[i]);
    }
    std::cout <<"\n" << __func__ << "\n";
#endif
  return h; // allow for copy-elision w/ O3 (std::move casting blocks it)
}

std::unique_ptr<uint8_t[]> sign_block(uint8_t *serialized_block, size_t blk_sz,
                                      int id) {

  std::string plain_text(reinterpret_cast<char *>(serialized_block), blk_sz);
  auto [signature, sz] = sign_message(privateKey, plain_text);
  if (sz != k_signature_size) {
    fmt::print("[{}] signature sizes are not matching\n", __func__);
  }
  auto h = std::make_unique<uint8_t[]>(k_signature_size + blk_sz);
  ::memcpy(h.get(), signature.get(), k_signature_size);
  ::memcpy(h.get() + k_signature_size, serialized_block, blk_sz);
  return h; // allow for copy-elision w/ O3 (std::move casting blocks it)
}

bool validate_signature(uint8_t *signed_block, size_t block_sz, int node_id) {
  // unmarhal the signed block
  auto signature = std::make_unique<uint8_t[]>(k_signature_size);
  auto msg = std::make_unique<uint8_t[]>(block_sz - k_signature_size);
  ::memcpy(signature.get(), signed_block, k_signature_size);
  ::memcpy(msg.get(), signed_block + k_signature_size,
           block_sz - k_signature_size);

  std::string plain_text(reinterpret_cast<char *>(msg.get()),
                         block_sz - k_signature_size);
  bool authentic = verify_signature(publicKey, plain_text,
                                    reinterpret_cast<char *>(signature.get()),
                                    k_signature_size);
  return authentic;
}
