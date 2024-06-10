#include "networking.hpp"
#include "protocol.hpp"

class node {
public:
  node(int id, int epoch,
       std::map<int, std::unique_ptr<network_connection>> &conns)
      : connections(conns) {
    node_id = id;
    cur_epoch = epoch;
    chain = std::make_unique<blockchain>();
#if 0
    std::cout << __func__ << " Am I leader (node=" << node_id
              << ") for this epoch=" << epoch << "? "
              << protocol::is_leader(node_id, epoch) << "\n";
#endif
  }

  // invoked every time a 'correct' proposer will propose
  void set_epoch(int e) { cur_epoch = e; }

  // proposes a tx (leader)
  void propose(uint8_t *txs, size_t txs_sz, int epoch) {
    cur_epoch = epoch;
    block blk(cur_epoch, txs, txs_sz, node_id);

    protocol::propose(cur_epoch, blk, chain, connections, node_id);
  }

  // polls for incomming msgs (all nodes run this function)
  void poll() {
    block blk = protocol::incomming_msg(cur_epoch, node_id, connections);
    if (blk.is_empty()) {
      return;
    }
    // we have received a new validated message that has been send by an
    // authorized node (either leader or follower) in the current epoch
    // the protocol does not differentiate between vote-msgs and propose-msgs
    // in the end for a block to become final it must have been proposed by the
    // epoch-leader and be voted by at least the 2N/3 Byz leaders can equivocate
    // and double-sent messages but a node only votes for the first block they
    // have seen for first time in each epoch equivocation is not a problem as
    // if at least 2n/3 see the same block this will be notarized else
    // not-conflicting blocks for the same epoch will be notarized.

    // Case #1: the message is a proposal from the leader and I am a follower
    // that might vote for that
    int org_sender = blk.sender_node_id;
    int leader_node = leader_for_epoch(cur_epoch);
    if ((org_sender == leader_node) && (node_id != leader_node)) {
      vote(blk);
    }

    // Case #2: the message is a vote from a follower
    if ((org_sender != leader_node)) {
      // both the followers and the leader will append the vote to the pending
      // blocks in case this is a vote to a (slower) follower that has not yet
      // received the proposal, buffer it
      if (chain->block_extends_chain(blk, leader_node, node_id) ==
          blockchain::kBlock_added) {
        fmt::print("[{}] Node={} receives a message from a node other than the "
                   "leader (org_sender={}) so let's echo\n",
                   __func__, node_id, org_sender);

        blk.sender_node_id = node_id;
        auto cur_serialized_block = blk.serialize();
        auto s_msg = sign_block(std::get<0>(cur_serialized_block).get(),
                                std::get<1>(cur_serialized_block), node_id);
        auto data_sz = chain->get_signed_block_size();

        protocol::bcast_msg(connections, node_id, s_msg.get(), data_sz);
      }
      // update potentially notarized chain
      chain->finalize_blocks();
    }
  }

  // vote for a tx (only nodes w/o leadership vote)
  void vote(block &blk) {
    protocol::vote(cur_epoch, blk, chain, connections, node_id);
  }

  // getters
  std::unique_ptr<blockchain> &get_chain() { return chain; }

  int get_node_id() { return node_id; }

private:
  std::unique_ptr<blockchain> chain;
  int node_id, cur_epoch;
  std::map<int, std::unique_ptr<network_connection>> &connections;
};
