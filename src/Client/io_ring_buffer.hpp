#pragma once
#include <array>
#include <bit>
#include <cstddef>
#include <ev.h>
#include <sys/uio.h>
#include <cassert>

struct prepare_t {
  std::size_t prepared_iovecs {};
  std::array<iovec, 2> iovec_array{};
  prepare_t() = default;
public:
  prepare_t (std::size_t valid_size, void* iovec1, std::size_t iovec1_len, void* iovec2, std::size_t iovec2_len)
  : prepared_iovecs (valid_size)
  , iovec_array { iovec{.iov_base=iovec1, .iov_len=iovec1_len}, iovec{.iov_base=iovec2, .iov_len=iovec2_len} }
  {}
  std::size_t prepared_bytes() const { return iovec_array[0].iov_len + iovec_array[1].iov_len;}
  bool empty() const { return prepared_iovecs == 0;}
};

template <std::size_t N> class io_ring_buffer {

  static_assert(N!=0, "tcp_buffer size cannot be zero");
  std::size_t read{0}, write{0};
  std::size_t readable{0};
  std::array<std::byte, N> buffer;
  std::size_t mask(std::size_t);

public:

  bool empty();
  bool full();
  io_ring_buffer() = default;
  prepare_t prepare_read();
  prepare_t prepare_write();
  void commit_read(std::size_t);
  void commit_write(std::size_t);
  std::size_t r_available();
  std::size_t w_available();
  void reset();
};

template <std::size_t N> std::size_t io_ring_buffer<N>::mask(std::size_t idx) {
  if constexpr (std::has_single_bit(N)) {
    return idx & (N-1);
  } else {
    return idx % N;
  }
}

template <std::size_t N> bool io_ring_buffer<N>::empty() {
  return readable == 0;
}

template <std::size_t N> bool io_ring_buffer<N>::full() {
  return readable == N;
}

template <std::size_t N> prepare_t io_ring_buffer<N>::prepare_read() {
  if (empty()) {
    return prepare_t(0, nullptr, 0, nullptr, 0);
  }

  if ( (read < write) || (full() && read==0) ) {
    return prepare_t(1, &buffer[read], readable, nullptr , 0);
  }

  //else ( (read > write) || (full() && read >0) )
  return prepare_t(2, &buffer[read], N - read, &buffer[0], write);
}

template <std::size_t N> prepare_t io_ring_buffer<N>::prepare_write() {
  if (full()) {
    return prepare_t(0, nullptr, 0, nullptr, 0);
  }

  if ( (write < read) || (empty() && write == 0) ) {
    return prepare_t(1, &buffer[write], w_available(), nullptr, 0);
  }

  // else ( (write > read) || (empty() && write >0) )
  return prepare_t(2, &buffer[write], N - write, &buffer[0], read);
}

template <std::size_t N> void io_ring_buffer<N>::commit_read(std::size_t bytes) {
  if (bytes == 0) return;
  // wraparound guard.
  assert(bytes <= r_available());
  read = mask(read+bytes);
  readable -= bytes;
}

template <std::size_t N> void io_ring_buffer<N>::commit_write(std::size_t bytes) {
  if (bytes == 0) return;
  // wraparound guard.
  assert(bytes <= w_available());
  write = mask(write+bytes);
  readable += bytes;
}

template <std::size_t N> std::size_t io_ring_buffer<N>::r_available() {
  return readable;
}

template <std::size_t N> std::size_t io_ring_buffer<N>::w_available() {
  return N - readable;
}

template <std::size_t N> void io_ring_buffer<N>::reset() {
  read = 0;
  write = 0;
  readable = 0;
}
