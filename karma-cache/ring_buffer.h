#pragma once
#include <cstdint>
/*
    并发不安全
    但我们是share nothing，倒也无所谓
*/
// namespace cache {
template <typename T, uint32_t Capacity> class RingBuffer {
public:
  void Push(T item) {
    assert(Size() < Capacity);
    data_[tail_ % Capacity] = item;
    tail_ += 1;
  }

  T Pop() {
    assert(Size() > 0);
    return data_[head_++ % Capacity];
  }

  uint32_t Size() const { return tail_ - head_; };
  uint32_t IsFull() const { return Size() == Capacity; }

private:
  T data_[Capacity];
  uint64_t head_ = 0;
  uint64_t tail_ = 0;
};
// }