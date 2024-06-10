#include "node.hpp"
#include <atomic>
#include <chrono>
#include <fmt/printf.h>
#include <thread>
#include <vector>
#include <mutex>
using namespace std::chrono;

std::map<int, std::unique_ptr<network_connection>> connections;
static uint8_t txs[k_payload_size];
static std::atomic<int> global_epoch = 1;
static std::mutex print_mtx;

void initialize_connections() {
  for (auto i = 0ULL; i < kNodes; i++) {
    auto ptr_c = std::make_unique<network_connection>();
    connections.insert({i, std::move(ptr_c)});
  }
}

void print_connections() {
  for (auto &elem : connections) {
    fmt::print("{} ", elem.first);
    (elem.second)->print();
  }
}

static void timer_func() {
  static auto start = high_resolution_clock::now();
  for (;;) {
    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<seconds>(stop - start);
    if (duration.count() >= kEpochDuration) {
      global_epoch.fetch_add(1);
      fmt::print("NEW EPOCH={}\n", global_epoch.load());
      start = high_resolution_clock::now();
    }
    if (global_epoch.load() == kFinalEpochVal)
        return;
  }
}

int new_epoch() { return global_epoch.load(); }

static void leader_func(node *n) {
  auto cur_epoch = new_epoch();
  n->propose(txs, k_payload_size, cur_epoch);
  for (;;) {
    if (new_epoch() != cur_epoch)
      return;
    n->poll();
  }
}

static void follower_func(node *n) {
  auto cur_epoch = new_epoch();
  n->set_epoch(cur_epoch);
  for (;;) {
    if (new_epoch() != cur_epoch)
      return;
    n->poll();
  }
}

static void thread_func(node *n) {
  for (auto i = 0ULL; i < kRounds; i++) {
    fmt::print("[{}] *******round={}*******\n\n\n\n", __func__, i);
    if (protocol::is_leader(n->get_node_id(), new_epoch())) {
      fmt::print("[{}] node w/ id={} is the leader for round={}\n", __func__,
                 n->get_node_id(), i);
      leader_func(n);
    } else {
      fmt::print("[{}] node w/ id={} is a follower for round={}\n", __func__,
                 n->get_node_id(), i);
      follower_func(n);
    }
  }
  // each node prints each view of the chain before exiting for good
  std::lock_guard<std::mutex> lk(print_mtx);
  fmt::print("[{}] node={} exits\n", __func__, n->get_node_id());
  fmt::print("*----*----**----*----**----*----**----*----**----*----**----*----*");
  n->get_chain()->print();
  fmt::print("*----*----**----*----**----*----**----*----**----*----**----*----*");
}

int main(void) {
  initialize_connections();
  ::memset(txs, 'd', k_payload_size);

  std::thread timer(timer_func);

  node leader(0, 1, connections);
  node f1(1, 1, connections);
  node f2(2, 1, connections);
  node f3(3, 1, connections);
  std::vector<std::thread> threads;
  threads.emplace_back(thread_func, &f1);
  threads.emplace_back(thread_func, &f2);
  threads.emplace_back(thread_func, &f3);

  threads.emplace_back(thread_func, &leader);

  for (auto &t : threads) {
    t.join();
  }

  timer.join();

  // should not be any outstanding messages in this case
  print_connections();
    return 0;
}
