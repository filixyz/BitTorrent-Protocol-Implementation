#ifndef TRNSFR_TYPES
#define TRNSFR_TYPES

#include <algorithm>
#include <cassert>
#include <concepts>
#include <cstdint>
#include <iterator>
#include <optional>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <vector>
#include <cstddef>
#include "DynamicBitset.hpp"
#include <chrono>
#include "Constants.hpp"

struct piece_request_t {


  std::uint32_t piece_index;
  std::uint32_t block_index;

  // arrival time is caluclated transiently at call site.
  // the difference of the request time from that is that pieces RTT
  //
  std::chrono::time_point<std::chrono::steady_clock> request_time;

  explicit piece_request_t (std::uint32_t p, std::uint32_t b): piece_index(p), block_index(b) {}


};

class block_requests_list_t {

  static_assert(sizeof(piece_request_t) == 16 and std::is_trivially_copyable_v<piece_request_t>);

  std::size_t sent_count {0};
  std::vector<piece_request_t> request_list;

public:

  using rtt_duration = std::chrono::microseconds;

  block_requests_list_t() { request_list.reserve(bprotocol::constants::max_block_limit() / 4); }

  bool push_block_request ( std::uint32_t piece, std::uint32_t block ) {

    if ( request_list.size() >= bprotocol::constants::max_block_limit() ) return false;

    request_list.emplace_back(piece, block);

    return true;

  }

  bool unsent_requests_exist() const {

    return (request_list.size() > sent_count);

  }

  // Safe Usage:
  // while (unsent_requests_exist()) {
  //    auto request = get_head_request();
  //    // dispatch or do whatever here
  // }
  //
  // if get_head_request() is invoked and non exists, or
  // inner vector is empty accessing returned reference is UB.
  //
  piece_request_t get_head_request() const {

    assert(unsent_requests_exist());

    return request_list[sent_count];

  }

  void mark_head_sent() {

    assert(unsent_requests_exist());

    request_list[sent_count].request_time = std::chrono::steady_clock::now();

    ++sent_count;

  }

  // This function removes the just recived block from the peers request list
  // and returns the rtt the block experienced on link, if not found return is nullopted
  //
  [[nodiscard]] std::optional<rtt_duration> pop_recieved_block (std::uint32_t piece, std::uint32_t block) {

    auto start = request_list.begin();
    auto last  = std::next(start, sent_count);

    auto found = std::find_if (start, last, [&](const piece_request_t& request){
      return
        request.piece_index == piece && request.block_index == block;
    });

    if (found == last) return std::nullopt; // not found.

    rtt_duration rtt = std::chrono::duration_cast<rtt_duration> (
        std::chrono::steady_clock::now() - (*found).request_time
    );

    // swap remove here.
    // Faster execution in theory, but breaks, order in which pieces, where delegated to be
    // requested, this might lead to pieces in higher priority being requested later than lesser
    // priority ones.
    //
    // Example:
    //
    //    --sent_count;
    //    auto boundary = request_list.begin() + sent_count;
    //    std::iter_swap(found, boundary);
    //    std::iter_swap(boundary, std::prev(request_list.end()));
    //    request_list.pop_back();
    //
    // But since objects are 16bytes and are at max 1024 in count, std::vector::erase() will suffice.
    //
    request_list.erase(found);

    --sent_count;

    return rtt;

  }

};

struct block_slot_t {

};

enum class piece_state_t: std::uint8_t {
  completed,
  hashing,
  inflight,
  free
};

enum class block_state_t: std::uint8_t {
  requested,
  unrequested,
  recieved,
};


struct piece_downloading_t {
  std::size_t piece;
  std::vector<block_state_t> block_map {block_state_t::unrequested};
  block_slot_t piece_storage;
};

struct piece_t {

  std::size_t    rank  {0}; // rank is equal to this pieces peers count
  std::size_t    rank_index;
  piece_state_t  state {piece_state_t::free};

};

namespace static_rank_partitions {

  static constexpr std::size_t count                  {2};
  static constexpr std::size_t seed_space_index       {0};
  static constexpr std::size_t inflights              {1};

};

using download_set_t = std::unordered_set <std::size_t, piece_downloading_t>;

class piece_picker_t {

  std::vector<piece_t>      piece_map;
  std::vector<std::size_t>  ranked_pieces;
  std::vector<std::size_t>  rank_partition;
  download_set_t            download_set;

public:

  piece_picker_t(std::size_t piece_count)
    : piece_map(piece_count), ranked_pieces(piece_count), rank_partition( static_rank_partitions::count )
  {}

  void increase_piece_rank(std::size_t);   // for haves
  void decrease_piece_rank(std::size_t);   // for disconnects and such
  bool rebuild_on_new_bitfield(DynamicBitset&);   // for new connects bitfield message
  void rebuild_on_dead_bitfield(DynamicBitset&, bool is_seeder); // on peer disconnect
  void assign_blocks();
  void reclaim_assigned_blocks();
  void block_completion_notification(std::size_t piece, std::size_t block_index);

};

#endif
