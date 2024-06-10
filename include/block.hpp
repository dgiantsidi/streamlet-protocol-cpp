#pragma once

#include <cstring>
#include <iostream>
#include <map>
#include <memory>
#include <stdint.h>
#include <tuple>
#include <vector>

#include "config.hpp"
#include "crypto.hpp"

// the 'block' of the blockchain

struct block {
  // constructors
  explicit block(int e, uint8_t *h, uint8_t *payload);
  explicit block(int e, uint8_t *h, uint8_t *payload, int node_id);
  explicit block(int e, uint8_t *payload, size_t payload_sz, int node_id);

  // copy-constructor
  block(const block &other);

  // move-constructor
  block(block &&other);

  // copy/move operators
  block &operator=(const block &other);
  block &operator=(block &&other);

  void print() {
    std::cout << __PRETTY_FUNCTION__ << " epoch=" << epoch << "\n";
    for (auto i = 0ULL; i < k_hash_size; i++) {
      std::cout << static_cast<int>(parent_h[i]);
    }
    std::cout << "\n";
  }

  // can be invoked w/o object initializations as they are state-less
  static size_t get_signed_block_size();
  static uint64_t get_signed_msg_epoch(uint8_t *signed_msg);
  static uint64_t get_signed_msg_sender(uint8_t *signed_msg);
  static uint64_t get_signed_msg_chain_id(uint8_t *signed_msg);
  static block construct_block(uint8_t *signed_msg);

  bool is_empty() { return epoch == 0; }

  // check if two blocks are equal
  bool equals(const block &other) {
    if (::memcmp(parent_h, other.parent_h, k_hash_size) != 0)
      return false;
    if (::memcmp(txs, other.txs, k_payload_size) != 0)
      return false;
    return (epoch == other.epoch) && (chain_id == other.chain_id);
  }

  // serialized block into a buffer of bytes/uint8_t
  std::tuple<std::unique_ptr<uint8_t[]>, size_t> serialize();

  // struct members
  uint64_t epoch;
  uint8_t parent_h[k_hash_size];
  uint8_t txs[k_payload_size];
  // these are metadata that are also serialized when signing and sending a
  // block
  // FIXME: the chain_id is not used anymore
  // the sender_node_id could be ommitted as digital signatures offer
  // transferable authentication--however we use the same PK pair for simplicity
  // so we use the sender_node_id to navigate on how to update the block
  // metadata (voters, etc.) in the blockchain
  int sender_node_id = -1;
  int chain_id = -1;
};
