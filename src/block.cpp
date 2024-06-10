
#include <cstring>
#include <iostream>
#include <map>
#include <memory>
#include <stdint.h>
#include <tuple>
#include <vector>

#include "config.hpp"

#include "block.hpp"

block::block(int e, uint8_t *h, uint8_t *payload) {
  if (e == 0) {
    // genesis block
    epoch = e;
    ::memset(parent_h, '0', k_hash_size);
    ::memset(txs, '0', k_payload_size);
  } else {
    epoch = e;
    ::memcpy(parent_h, h, k_hash_size);
    ::memcpy(txs, payload, k_payload_size);
  }
}

block::block(int e, uint8_t *h, uint8_t *payload, int node_id) {
  if (e == 0) {
    // genesis block
    epoch = e;
    ::memset(parent_h, '0', k_hash_size);
    ::memset(txs, '0', k_payload_size);
  } else {
    epoch = e;
    ::memcpy(parent_h, h, k_hash_size);
    ::memcpy(txs, payload, k_payload_size);
    sender_node_id = node_id;
  }
}

size_t block::get_signed_block_size() {
  return (sizeof(uint64_t) + k_hash_size * sizeof(uint8_t) +
          k_payload_size * sizeof(uint8_t) + sizeof(int) + sizeof(int) +
          k_signature_size);
}

block::block(int e, uint8_t *payload, size_t payload_sz, int node_id) {
  if (e == 0) {
    // genesis block
    epoch = e;
    ::memset(parent_h, '0', k_hash_size);
    ::memset(txs, '0', k_payload_size);
  } else {
    epoch = e;
    sender_node_id = node_id;
    ::memcpy(txs, payload, k_payload_size);
  }
}

block::block(const block &other) {
  epoch = other.epoch;
  sender_node_id = other.sender_node_id;
  chain_id = other.chain_id;
  ::memcpy(parent_h, other.parent_h, k_hash_size);
  ::memcpy(txs, other.txs, k_payload_size);
}

block::block(block &&other) {
  epoch = other.epoch;
  sender_node_id = other.sender_node_id;
  chain_id = other.chain_id;
  ::memcpy(parent_h, other.parent_h, k_hash_size);
  ::memcpy(txs, other.txs, k_payload_size);
}

// copy/move operators
block &block::operator=(const block &other) {
  epoch = other.epoch;
  sender_node_id = other.sender_node_id;
  chain_id = other.chain_id;
  ::memcpy(parent_h, other.parent_h, k_hash_size);
  ::memcpy(txs, other.txs, k_payload_size);
  return *this;
}

block &block::operator=(block &&other) {
  epoch = other.epoch;
  sender_node_id = other.sender_node_id;
  chain_id = other.chain_id;
  ::memcpy(parent_h, other.parent_h, k_hash_size);
  ::memcpy(txs, other.txs, k_payload_size);
  return *this;
}

uint64_t block::get_signed_msg_epoch(uint8_t *signed_msg) {
  uint64_t epoch = 0;
  int offset = k_signature_size;
  ::memcpy(&epoch, signed_msg + offset, sizeof(uint64_t));
  return epoch;
}

uint64_t block::get_signed_msg_sender(uint8_t *signed_msg) {
  uint64_t sender = -1;
  int offset = k_signature_size + sizeof(uint64_t) +
               k_hash_size * sizeof(uint8_t) + k_payload_size * sizeof(uint8_t);
  ::memcpy(&sender, signed_msg + offset, sizeof(int));
  return sender;
}

uint64_t block::get_signed_msg_chain_id(uint8_t *signed_msg) {
  uint64_t chain_id = -1;
  int offset = k_signature_size + sizeof(uint64_t) +
               k_hash_size * sizeof(uint8_t) +
               k_payload_size * sizeof(uint8_t) + sizeof(int);
  ::memcpy(&chain_id, signed_msg + offset, sizeof(int));
  return chain_id;
}

block block::construct_block(uint8_t *signed_msg) {
  auto *ptr = signed_msg + k_signature_size;

  int epoch = get_signed_msg_epoch(signed_msg);
  int org_sender = get_signed_msg_sender(signed_msg);
  int chain_id = get_signed_msg_chain_id(signed_msg);
  //  std::cout << __func__ << " epoch=" << epoch << " from sender=" <<
  //  org_sender
  //            << "\n";
  block blk(epoch, ptr + sizeof(uint64_t),
            ptr + sizeof(uint64_t) + k_hash_size);
  blk.sender_node_id = org_sender;
  blk.chain_id = chain_id;
  return blk;
}

std::tuple<std::unique_ptr<uint8_t[]>, size_t> block::serialize() {
  auto data_sz = sizeof(uint64_t) + k_hash_size * sizeof(uint8_t) +
                 k_payload_size * sizeof(uint8_t) + sizeof(int) + sizeof(int);

  std::unique_ptr<uint8_t[]> ptr = std::make_unique<uint8_t[]>(data_sz);
  auto offset = 0;
  // std::cout << __PRETTY_FUNCTION__ << ": epoch=" << epoch << "\n";
  ::memcpy(ptr.get() + offset, &epoch, sizeof(uint64_t));
  offset += sizeof(uint64_t);
  ::memcpy(ptr.get() + offset, parent_h, k_hash_size * sizeof(uint8_t));
  offset += k_hash_size * sizeof(uint8_t);
  ::memcpy(ptr.get() + offset, txs, k_payload_size * sizeof(uint8_t));
  offset += k_payload_size * sizeof(uint8_t);
  ::memcpy(ptr.get() + offset, &sender_node_id, sizeof(int));
  offset += sizeof(int);
  ::memcpy(ptr.get() + offset, &chain_id, sizeof(int));
  return std::make_tuple(std::move(ptr), data_sz);
}
