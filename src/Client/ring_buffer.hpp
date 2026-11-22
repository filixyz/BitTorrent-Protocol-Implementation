#ifndef RANGE_BUFFER
#define RANGE_BUFFER

#include <cstddef>
#include <array>
#include <utility>

template < typename T, std::size_t N > class ring_buffer {
  static_assert( N != 0 );

  std::array<T, N> cache;
  std::size_t write{0};
  std::size_t read{0};
  std::size_t length{0};

  std::size_t plus_mask(std::size_t num) {
    return (num + 1 == N) ? 0 : ++num;
  }

public:

  class readable_range_t {

    ring_buffer<T, N> & buffer;

  public:

    readable_range_t(ring_buffer<T, N>& _buffer) : buffer(_buffer) {}

    struct iterator {

      iterator& operator++() {
        ++traversed;
        return *this;
      }

      T& operator*() const {

        std::size_t current = buffer.read + traversed;

        if (current >= N) current = current - N;

        return buffer.cache[current];

      }

      bool operator==(const iterator& other) const {
        return
          &buffer   == &other.buffer        &&
          traversed == other.traversed;
      }

      iterator( ring_buffer<T,N>& _buffer ): buffer(_buffer) {}

    private:
      ring_buffer<T, N>& buffer;
      std::size_t traversed {0};
      friend class readable_range_t;
    };

    iterator begin() const {

      iterator start (buffer);
      start.traversed = 0;
      return start;

    }

    iterator end() const {

      iterator ending (buffer);
      ending.traversed = buffer.length;
      return ending;

    }

  };

  ring_buffer() = default;

  bool push (T val) noexcept {

    if (length == N)
      return false;

    cache[write] = std::move(val);
    write = plus_mask(write);
    ++length;

    return true;

  }

  bool pop (T& val) noexcept {
    if (empty())
      return false;
    val = std::move(cache[read]);
    --length;
    read = plus_mask(read);
    return true;
  }

  bool empty() const noexcept {
    return length == 0;
  }

  void clear () noexcept {
    write = 0;
    read = 0;
    length = 0;
  }

  std::size_t size() const noexcept  {
    return length;
  }

  // can break fifo if smart tricks are played
  //
  readable_range_t readable_range() {
    return readable_range_t{*this};
  }

  // if .empty() is true :
  // the object at read index of the cache array is returned
  // which should be logically garbage data so if it shoots
  // you in the leg thats on you (being stupid)
  //
  T& front() {
    return cache[read];
  };

  // discard_pop within readable_range breaks range semantics
  // if this is something to be done exit the range forloop
  // immediately discard_pop and your logic for that iteration
  // is invoked, or else entries will be skipped.
  //
  void discard_pop() {

    if (empty()) return;

    --length;

    read = plus_mask(read);

  };

};

#endif
