#ifndef TRNSFR_TYPES
#define TRNSFR_TYPES

#include <cstdint>
#include <queue>
#include <set>
#include <unordered_set>
#include <vector>
#include <cstddef>
#include "DynamicBitset.hpp"
#include "PeerManagerTypes.hpp"

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
  std::vector<std::size_t>  pieces;
  std::vector<std::size_t>  rank_partition;
  download_set_t            download_set;

public:

  piece_picker_t(std::size_t piece_count)
    : piece_map(piece_count), pieces(piece_count), rank_partition( static_rank_partitions::count )
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
