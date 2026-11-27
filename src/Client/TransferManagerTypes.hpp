#ifndef TRNSFR_TYPES
#define TRNSFR_TYPES

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iterator>
#include <optional>
#include <ranges>
#include <type_traits>
#include <unordered_set>
#include <vector>
#include <cstddef>
//#include "DynamicBitset.hpp"
#include <chrono>
#include "Constants.hpp"
#include <iostream>

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

  static_assert(sizeof(piece_request_t) == 16 and  std::is_trivially_copyable_v<piece_request_t>);

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


namespace rank_partitions {

  static constexpr std::size_t static_count           {2};
  static constexpr std::size_t inflights              {0};
  static constexpr std::size_t seed_space_index       {1};

};

struct piece_t {

  std::size_t    peer_count{0};
  std::size_t    rank_index;
  piece_state_t  state {piece_state_t::free};

};

struct piece_picker_t {

  using downloading_set_t = std::unordered_set <std::size_t, piece_downloading_t>;

  std::vector<piece_t>      piece_map;
  std::vector<std::size_t>  ranked_pieces;
  std::vector<std::size_t>  rank_partition;
  downloading_set_t         download_set;

  // bool next_parition_range_null(std::size_t current_index) {

  //   std::size_t partition_value = rank_partition[current_index];

  //   std::size_t next_partition_value =
  //     current_index + 1 < rank_partition.size() ? rank_partition[current_index + 1 ] :  partition_value;

  //   // if a differece exists that means current partition is not empty

  //   return next_partition_value - partition_value;

  // }

  bool paritition_exists(std::size_t current_index) {

    return (rank_partition[current_index] < ranked_pieces.size());

  }

  bool partition_has_pieces(std::size_t current_index) {

    if (!paritition_exists(current_index)) return false;

    return rank_partition[current_index+1] - rank_partition[current_index];

  }

  std::optional<std::size_t> next_partiton_with_pieces_from (std::size_t partition) {

    for(std::size_t current = partition + 1 ; current < rank_partition.size(); ++current) {

      if (partition_has_pieces(current)) return current;

    }

    return std::nullopt;

  }

  std::optional<std::size_t> prev_partition_with_pieces_from(std::size_t partition) {

    if (partition <= rank_partitions::seed_space_index) return std::nullopt;

    for (std::size_t current = partition - 1 ; current >=  rank_partitions::seed_space_index; --current) {

      if (partition_has_pieces(current)) return current;

    }

    return std::nullopt;

  }

  static std::size_t get_rank(piece_t& piece) {

    // Since peer ranks are determeined by thier peer count and
    // inflights and seed spaces already occupy the first two indexes
    // of the peratition table index 0, and 1 respectively, and recall that we
    // don't add ranks for seeders pieces, so a piece with a single leeching peer
    // that pieces rank should be situated after the seeds space index which is
    // 1 + pieces peer count AKA that pieces rank

    return piece.peer_count + rank_partitions::seed_space_index;

  }

public:

  piece_picker_t(std::size_t piece_count)
    : piece_map(piece_count), ranked_pieces(piece_count), rank_partition( rank_partitions::static_count )
  {

    for (std::size_t index : std::views::iota(std::size_t{0}, piece_count)) {
      piece_map[index].rank_index = index;
      ranked_pieces[index] = index;
    }

    rank_partition.push_back(piece_count);

  }

  // for haves
  void increase_piece_rank(std::size_t piece_index) {

    auto& piece = piece_map[piece_index];

    // const std::size_t old_rank = get_rank(piece);

    const std::size_t new_rank = ( piece.peer_count++, get_rank(piece) ); // increase peer_count then return new rank

    // objective: swap this piece position with last piece in old rank.

    //std::optional<std::size_t> next_partiiton_with_pieces = next_partiton_with_pieces_from(old_rank);

    // std::size_t last_piece_in_old_rank = -1 + ( next_partiiton_with_pieces
    //   ? rank_partition[next_partiiton_with_pieces.value()]
    //   : piece_map.size()
    // );

    std::size_t last_piece_in_old_rank = rank_partition[new_rank] - 1;

    std::size_t piece_old_rank_index = piece.rank_index;

    std::swap(ranked_pieces[piece_old_rank_index], ranked_pieces[last_piece_in_old_rank]);

    piece_map[ranked_pieces[last_piece_in_old_rank]] .rank_index = last_piece_in_old_rank;
    piece_map[ranked_pieces[piece_old_rank_index]]   .rank_index = piece_old_rank_index;

    // next objective adjust rank parittion table accordingly

    if (!paritition_exists(new_rank)) rank_partition.push_back(piece_map.size());

    // for (std::size_t stale_empty_value = rank_partition[new_rank], rank = new_rank; rank != std::numeric_limits<std::size_t>::max(); --rank) {

    //   if (rank_partition[rank] != stale_empty_value) break;


    // }

    rank_partition[new_rank] = last_piece_in_old_rank;

  }

  void decrease_piece_rank(std::size_t);   // for disconnects and such
  //bool rebuild_on_new_bitfield(DynamicBitset&);   // for new connects bitfield message
  //void rebuild_on_dead_bitfield(DynamicBitset&, bool is_seeder); // on peer disconnect
  void assign_blocks();
  void reclaim_assigned_blocks();
  void block_completion_notification(std::size_t piece, std::size_t block_index);

};

#endif
