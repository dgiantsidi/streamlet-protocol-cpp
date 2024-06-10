#pragma once
#include <cstring>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <tuple>

// thread-safe network queue that realizes the connection between two peers
class network_connection {
public:
  void send(uint8_t *msg, size_t sz);

  // returns true if there are messages on the queue
  bool has_msgs();

  // retrieves the received message
  using net_message = std::tuple<std::unique_ptr<uint8_t[]>, size_t>;
  net_message receive();

  // prints the number of messages in the message queue
  void print();

private:
  std::queue<net_message> message_queue;
  std::mutex mtx;
};