### Implementation of Streamlet protocol in cpp (https://eprint.iacr.org/2020/088.pdf)


## System dependencies
- 18.04.1-Ubuntu
- C++ 17.0
- libfmt
- make
- OpenSSL 1.1.1

## Build instructions
Build with `make` or `make DEBUG=1` to enable address sanitizer (detects memory leaks/violations).


## Code structure
 - `include`: header files
 - `src`: source files
 - `main.cpp`: a running example (non-Byzantine failures) of N=3f+1 (f=1), 4 nodes

 Next we explain each of the files:
 - the cryptographic signign and hash functions are in the `crypto.hpp`, `rsa_.h` (wrapper on top of RSA functions) and `crypto.cpp` respectively (RSA and SHA-256 from OpenSSL).
 - the message queue for the connections (thread-safe) is in the `networking.hpp` and `networking.cpp`.
 - the block structure is defined in `block.hpp` and `block.cpp`.
 - the blockchain data structure is defined in `ll_log.h` and it is implemented as a tree where the root node is the genesis block. Each node has an arbritary number of kids which express blocks that are appended in the blockchain. The blocks are not necessarily committed (final). Each block in the blockchain (expressed as `struct data_block`) has metadata which show its state (e.g. `is_final`, `is_notarized`, `voters` of this block, etc.). Eagerly adding the blocks to the blockchain does
        not violate safety since final blocks need to have been notarized first (ack-ed by at least 2N/3).
- the protocol library is implemented in `protocol.hpp` (Note: we could have decoupled the implementation from the declaration of the functions for readability) where we implement three core functionalities: 
    - (1) **propose** where the leader signs, appends to each blockchain and broadcasts the signed block to all other peers.
    - (2) **vote** where the follower broadcast the received proposal to all others.
    - (3) **incomming_messages** where all nodes listen for new votes/proposals and validate all incomming messages.
- the `node.hpp` describes the functionality of a node. The `node.hpp` alogn with the `protocol.hpp` implement the StreamLet protocol. The node polls, proposes and votes for messages. The vote and the propose operations are streightforward. Upon polling and receiving a valid message, the node updates its own blockchain if that message extends one of the longest notarized chains. If the message is from the leader (proposal) the node adds the message to its blockchain if it extends one of the longest notarized chains and bcasts to all other nodes. If the message is not from leader then the message is added to the chain if it extends a longest notarized chain and the node it broadcasts this vote to all other nodes (echo)---this is okay because we only finalize notarized messages where the leader has sent a proposal (so it is okay to eagerly update). Lastly, if the node has already an entry for this message in the blockchain, it updates teh metadata if needed.

**NOTEs**:
 - I have not implemented the extra credit tasks and the system has been tested in non-Byzantine failure scenarions.
 - The first task about the quorum certificates requires extensions to the `data_block` structure. We already store the `voters` (e.g., each node that has seen/voted for this block). We could also store the signatures of each of the voter.
 - About testing with Byzantine failures, I would create test for a Byzantine leader that equivocates during an epoch (or even propose multiple blocks at the same follower). The code should handle this but it is not tested under such scenarios.
 - I passed the code from address sanitizer and looks good (no memory violations, data races, etc.). I had tested the ll_log.h and rsa_.h funcitonality however I must have done minor updates on the APIs so the tests might not be up-to-date anymore.
 - It is not optimized for performance. I use BFS which when we finalize blocks we traverse the entire blockchain. As an optimization I could have used an index to only start from notarized blocks. I have only introduced an index to quickly spot the blocks in the blockchain for updating their metadata but it is memory bound at the moment.
