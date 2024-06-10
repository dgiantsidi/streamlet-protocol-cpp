#include "networking.hpp"

void network_connection::send(uint8_t *msg, size_t sz) {
  std::lock_guard<std::mutex> lk(mtx);
  auto ptr = std::make_unique<uint8_t[]>(sz);
  ::memcpy(ptr.get(), msg, sz);
  message_queue.push(std::make_tuple(std::move(ptr), sz));
}

bool network_connection::has_msgs() {
  std::lock_guard<std::mutex> lk(mtx);
  return !message_queue.empty();
}

network_connection::net_message network_connection::receive() {
  std::lock_guard<std::mutex> lk(mtx);
  auto &msg = message_queue.front();
  net_message ret_msg =
      std::make_tuple(std::move(std::get<0>(msg)), std::get<1>(msg));
  message_queue.pop();
  return ret_msg;
}

void network_connection::print() {
  std::lock_guard<std::mutex> lk(mtx);
  std::cout << message_queue.size() << " msgs\n";
}
