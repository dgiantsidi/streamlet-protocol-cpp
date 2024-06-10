#pragma once

#include "block.hpp"
#include "config.hpp"
#include "crypto.hpp"
#include <cmath>
#include <cstring>
#include <fmt/printf.h>
#include <iostream>
#include <map>
#include <memory>
#include <queue>
#include <set>
#include <stdint.h>
#include <tuple>
#include <vector>

// describes the blockchain which is model as a graph/tree
// where the root node is the genesis block and is the same accross all nodes
// each node can have many kids

class blockchain {
public:
  // constructor/destructor
  // we cleanup using a BFS
  blockchain() {
    log = new data_node(block(0, nullptr, nullptr), 0, true);
    // genesis block is final and same for all nodes
    log->is_final = 1;
  }

  void cleanup() {
    std::priority_queue<data_node *> q;
    q.push(log);
    std::vector<data_node *> vec;
    while (!q.empty()) {
      auto *ptr = q.top();
      q.pop();
      vec.push_back(ptr);

      for (auto &kid : ptr->kids) {

        q.push(kid);
      }
    }
    for (auto &v : vec) {
      delete v;
    }
  }

  ~blockchain() { cleanup(); }

  // the 'data_node' is the data-block that is inserted on the blockchain
  // it is a wrapper on top of the 'block' but also has some extra metadata
  // that are required
  // FIXME: NOTE: we did not optimized the code, there are quite a few "quick"
  // fixes such as not copying the block objects all the time but exchanging
  // ownership of a wrapper RAII-pointer
  struct data_node {
    explicit data_node(block other_b, int parent_length, bool proposed)
        : b(other_b) {
      length = parent_length + 1;
      is_proposed = proposed;
      voters.insert(b.sender_node_id);
    };
    block b;
    int length = 0;
    bool is_final = false;
    bool is_notarized = false;
    bool is_proposed = false;
    std::set<int> voters;
    std::vector<data_node *> kids;
    std::string print() {
      std::string os;
      os = "block w/ [e=" + std::to_string(b.epoch) +
           ", length=" + std::to_string(length) +
           ", is_final=" + std::to_string(is_final) +
           ", is_notarized=" + std::to_string(is_notarized) +
           ", is_proposed=" + std::to_string(is_proposed) + "]\n";
      return os;
    }
  };

  size_t get_signed_block_size() { return block::get_signed_block_size(); }

  // computes the parent hash of a new block based on the
  // longest_notarized_chain it finds first (in case there are many) then it
  // signs the block and returns it (it is called by a leader at the
  // propose-phase)
  std::unique_ptr<uint8_t[]> sign_new_block(int e, block &blk,
                                            int leader_node) {
    auto *block = longest_notarized_chain();
    // std::cout << block->print() << "\n";
    std::tuple<std::unique_ptr<uint8_t[]>, size_t> serialized_block =
        block->b.serialize();

    auto h = compute_hash(std::get<0>(serialized_block).get(),
                          std::get<1>(serialized_block));

    ::memcpy(blk.parent_h, h.get(), k_hash_size);
#if 0
    std::cout << __func__ << "\n";
    for (auto i = 0ULL; i < k_hash_size; i++) {
      std::cout << static_cast<int>(blk.parent_h[i]);
    }
    std::cout << "\n";
    for (auto i = 0ULL; i < k_hash_size; i++) {
      std::cout << static_cast<int>(h[i]);
    }
    std::cout << "\n";
#endif
    auto new_node =
        new data_node(blk, block->length, (blk.sender_node_id == leader_node));
    fmt::print("[{}] w/ sender_node_id={}\n", __func__, blk.sender_node_id);
    block->kids.push_back(new_node);
    blocks_map.insert({blk.epoch, new_node});

    // sign the block
    ::memcpy(blk.parent_h, h.get(), k_hash_size);
    auto cur_serialized_block = blk.serialize();
    return sign_block(std::get<0>(cur_serialized_block).get(),
                      std::get<1>(cur_serialized_block), leader_node);
  }

  // called periodically to finalize any blocks
  // we use a BFS to traverse the entire chain
  // Specifically, for each notarized or final block/node N in our workqueue,
  // visit all of its kids, if a kid is notarized then visit all the kids of
  // this kid (grandchildren of the initial node N) if any of the grandchildren
  // is notarized to then the kid and the node N become notarized.
  void finalize_blocks() {
    std::priority_queue<data_node *> q;
    q.push(log);
    while (!q.empty()) {
      auto *ptr = q.top();
      q.pop();
      auto parent_e = ptr->b.epoch;
      if (ptr->is_notarized || ptr->is_final) {
        for (auto &kid : ptr->kids) {
          if (kid->is_notarized && kid->b.epoch == (parent_e+1)) { // TODO::
            for (auto &grandchildern : kid->kids) {
              if (grandchildern->is_notarized && (grandchildern->b.epoch == (parent_e+2)) {
                kid->is_final = true;
                ptr->is_final = true;
              }
            }
            q.push(kid);
          }
        }
      }
    }
  }

  // we find the longest notarized chain and return the data_node
  // Similarly we use BFS to find the maximum length and store in a vector n all
  // potentially longest notarized chains In the vector we might also add not
  // only the global longest_notarized_chain but also the
  // longest_notarized_chains at the time of the search (before we reach the
  // global) Later on we only care about the blocks that are of max_l (maximum
  // lenght).
  data_node *longest_notarized_chain() {
    std::priority_queue<data_node *> q;
    q.push(log);

    std::vector<data_node *> n;
    // n.push_back(log);
    int max_l = -1; // log->length;
    while (!q.empty()) {
      auto *ptr = q.top();
      q.pop();
      bool flag = false;
      for (auto &kid : ptr->kids) {
        if (kid->is_notarized) {
          flag = true;
          max_l = (max_l <= kid->length) ? kid->length : max_l;
          q.push(kid);
        }
      }



      if (flag == false && max_l <= ptr->length) {
        n.push_back(ptr);
        max_l = ptr->length;
      }
    }

    for (auto &block : n) {
      if (block->length == max_l) {
        return block;
      }
    }
    fmt::print("[{}] we found none notarized chain\n", __func__);
    return nullptr;
  }

  enum ret_code { kBlock_exists = 0, kBlock_added = 1, kBlock_discarded = 2 };

  std::string get_voters(const std::set<int> &s) {
    std::string st;
    for (auto &elem : s) {
      st += std::to_string(elem) + " ";
    }
    return st;
  }

  size_t get_quorum() {
    float q = (2 * kNodes * 1.00) / 3.0;// N=6 -> 4, N == 3f+1 -> 6 = 3f+ 1 -> f = 5/3 -> f =1 

    return std::ceil(q); // 4 
  }

  ret_code block_extends_chain(block &blk, int leader_node, int cur_node) {
    // is the block already seen
    if (blocks_map.find(blk.epoch) != blocks_map.end()) {
      if (blocks_map[blk.epoch]->b.equals(blk)) {
        fmt::print("[{}] Node={} -> sender_node={} just sent us a block/vote "
                   "from epoch={} "
                   "we have already seen\n",
                   __func__, cur_node, blk.sender_node_id, blk.epoch);
        blocks_map[blk.epoch]->voters.insert(blk.sender_node_id);
        if (blk.sender_node_id == leader_node)
          blocks_map[blk.epoch]->is_proposed = true;
        // We only mark as notarized proposed blocks (faster voters can vote for
        // the proposal before a node receives the proposal form the leader
        // itself so the block is eligible for being notarized only when the
        // leader's proposal is received). Potential improvement: the voters'
        // might include in the block the leader's signature and all signatures
        // from other voters so that a follower can verify this vote is valid,
        // etc. matches to a leader's proposal and also notarize immediately if
        // that is the case)
        blocks_map[blk.epoch]->is_notarized =
            ((blocks_map[blk.epoch]->voters.size() >= get_quorum())) &&
            (blocks_map[blk.epoch]->is_proposed);
        fmt::print("[{}] block.epoch={} became notarized={} in node={} w/ "
                   "voters={} (quorum={})\n",
                   __func__, blk.epoch, blocks_map[blk.epoch]->is_notarized,
                   cur_node, get_voters(blocks_map[blk.epoch]->voters),
                   get_quorum());
      }
      // we only vote for the first block seen in that epoch that extends the
      // chains
      return ret_code::kBlock_exists;
    }

    // if the block has not been seen look if it extends the longest seen
    // notarized_chain() NOTE: refactoring-here it actually invokes part of the
    // longest_notarized_chain()
    std::priority_queue<data_node *> q;
    q.push(log);

    std::vector<data_node *> n;
    int max_l = -1;
    while (!q.empty()) {
      auto *ptr = q.top();
      q.pop();
      bool flag = false;
      for (auto &kid : ptr->kids) {
        if (kid->is_notarized) {
          flag = true;
          max_l = (max_l <= kid->length) ? kid->length : max_l;
          q.push(kid);
        }
      }
      if (flag == false && max_l <= ptr->length) {
        n.push_back(ptr);
        max_l = ptr->length;
      }
    }

    // n has all the notarized maximum length blockchains
    for (auto &block : n) {
      if (block->length == max_l) {
        if (extends(blk, block->b)) {
          auto new_node = new data_node(blk, block->length,
                                        (blk.sender_node_id == leader_node));
          block->kids.push_back(new_node);
          blocks_map.insert({blk.epoch, new_node});
          return ret_code::kBlock_added;
        }
      }
    }
    std::cout << __func__ << " there is no block that extends the blockchain\n";
    return ret_code::kBlock_discarded;
  }

  // checks that the block extends the chain
  bool extends(block blk, block b) {
    // blk.print();
    // b.print();
    std::tuple<std::unique_ptr<uint8_t[]>, size_t> serialized_block =
        b.serialize();
    auto h = compute_hash(std::get<0>(serialized_block).get(),
                          std::get<1>(serialized_block));

    if (::memcmp(h.get(), blk.parent_h, k_hash_size) == 0) {
      return true;
    }
    // std::cout << "\n";
    std::cout << __func__ << " returns false\n";
    return false;
  }

  // debugging
  void print() {
    std::cout << __func__ << "\n";
    std::priority_queue<data_node *> q;
    q.push(log);
    std::cout << q.top()->b.epoch << "\n";

    while (!q.empty()) {
      auto *ptr = q.top();
      q.pop();
      for (auto &kid : ptr->kids) {
        std::cout << kid->print() << " (dad=" << ptr->b.epoch << ") \n";
        q.push(kid);
      }
      std::cout << "\n";
    }
  }

private:
  // the blocks_map is just an index to quickly identify blocks on the chain
  // while it is used as an 'optimization', we never clean it up so it grows
  // forever similarly to the 'log' (blockchain). however, map has O(1) access
  // so performance-wise should be okay we could enable cleanup for finalized
  // blocks that are not expected to receive any votes, etc.
  std::map<uint64_t, data_node *> blocks_map;

  // the actual blockchain
  struct data_node *log;
};
