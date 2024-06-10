#include "crypto.hpp"
#include "ll_log.h"
#include "networking.hpp"
#include <fmt/printf.h>

namespace protocol {

bool is_leader(int node_id, int epoch) {
#if 0
  std::cout << __func__ << " node_id=" << node_id << " "
            << leader_for_epoch(epoch) << " ";
#endif
  return (node_id == leader_for_epoch(epoch));
}



// broadcasts a singed message to all nodes
void bcast_msg(std::map<int, std::unique_ptr<network_connection>> &connections,
               int my_id, uint8_t *msg, size_t msg_sz) {
  for (auto &connection_node : connections) {
    if (connection_node.first != my_id) {
      //  std::cout << __PRETTY_FUNCTION__ << " send to " <<
      //  connection_node.first
      //            << " w/ msg size=" << msg_sz << "\n";
      connection_node.second->send(msg, msg_sz);
    }
  }
}


// invoked by the leader; it signs and returns a new (proposed) block, the block
// is also appended to the blockchain (uncommitted)
std::unique_ptr<uint8_t[]> sign_and_append_block(block &blk, blockchain *ptr,
                                                 int leader) {
  return ptr->sign_new_block(blk.epoch, blk, leader);
}


// propose-phase executed by leader
void propose(int epoch, block &blk, std::unique_ptr<blockchain> &chain,
             std::map<int, std::unique_ptr<network_connection>> &connections,
             int my_id) {

  // (1) append the block to one of the longest notarized chain
  // (2) hash + sign the block w/ own key
  auto signed_blk =
      sign_and_append_block(blk, chain.get(), my_id); // I am the leader anyways
  auto data_sz = chain->get_signed_block_size();

  // (3) b-cast to the others
  bcast_msg(connections, my_id, signed_blk.get(), data_sz);
}

// vote-phase executed by followers
void vote(int epoch, block &blk, std::unique_ptr<blockchain> &chain,
          std::map<int, std::unique_ptr<network_connection>> &connections,
          int my_id) {
  // (1) validate that blk extends one of the longest notarized chain
  if (chain->block_extends_chain(blk, leader_for_epoch(epoch), my_id) ==
      blockchain::ret_code::kBlock_discarded) {
    fmt::print(
        "[{}] does not extend any of the longest notarized blockchains\n",
        __func__);
    return;
  }
  
  // (2) sign the block w/ own key
  blk.sender_node_id = my_id;
  auto [serialized_block, block_sz] = blk.serialize();
  auto data_sz = chain->get_signed_block_size();
  auto signed_blk = sign_block(serialized_block.get(), block_sz, my_id);

  fmt::print("[{}] Node={} votes for epoch={}\n", __func__, my_id, blk.epoch);
  // (3) b-cast the vote to everyone else
  bcast_msg(connections, my_id, signed_blk.get(), data_sz);
}


// this is called when polling for new messages: it returns a validated block (signed by the node who is supposed to be singed) 
// and the block epoch matches the current epoch
// else it is discarded and an empty block is returned instead
block incomming_msg(
    const int cur_epoch, int node_id,
    std::map<int, std::unique_ptr<network_connection>> &connections) {
  auto &my_queue = connections[node_id];

  // (1) check for incomming msgs in my network queue
  if (my_queue->has_msgs()) {
    auto [signed_msg, msg_sz] = my_queue->receive();

    // (2) validate the message is in the current epoch
    int msg_epoch = block::get_signed_msg_epoch(signed_msg.get());
    if (msg_epoch != cur_epoch) {
      std::cout << __PRETTY_FUNCTION__ << " wrong epoch (" << msg_epoch
                << " vs " << cur_epoch << ")\n";
      return block(0, nullptr, nullptr); // just return an empty block
    }
    /*
        std::cout << __PRETTY_FUNCTION__ << " correct epoch (" << msg_epoch
                  << " == " << cur_epoch << ")\n";
                  */

    int org_sender = block::get_signed_msg_sender(signed_msg.get());
    if (validate_signature(signed_msg.get(), msg_sz, org_sender))
      return block::construct_block(signed_msg.get());
    else {
      fmt::print("[{}] signatures invalid\n", __func__);
    }

  }
  return block(0, nullptr, nullptr); // just return an empty block
}

} // end namespace protocol
