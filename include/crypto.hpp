#pragma once

#include "config.hpp"
#include <functional>
#include <memory>

// returns the node_id that is the designated leader for that epoch
int leader_for_epoch(int epoch);

// computes and returns the SHA-256 of the serialized block
std::unique_ptr<uint8_t[]> compute_hash(uint8_t *serialized_block,
                                        size_t blk_sz);

// for generating verifiable digital signatures all nodes use the same PK pair
// (for simplicity) However, we pass the sender node_id (that signed the
// message) as argument to the sign_block()/validate_signature() functions so
// that we can support different PK pairs
std::unique_ptr<uint8_t[]> sign_block(uint8_t *serialized_block, size_t blk_sz,
                                      int node_id);

bool validate_signature(uint8_t *signed_block, size_t block_sz, int node_id);
