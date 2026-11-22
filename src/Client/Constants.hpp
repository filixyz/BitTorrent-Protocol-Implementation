#pragma once
#include <cstdint>
#include <sys/socket.h>
#include <cstddef>
#include <array>

namespace bprotocol::constants {

  consteval std::array<char, 20> get_client_id() {
    std::array<char, 20> id;
    const char id_with_null  [21] = "FJ0001-x4Kn8mR2pL9sq";
    for (auto i = 0; i < 20; ++i)
      id[i] = id_with_null[i];
    return id;
  }

  inline constexpr std::array <char, 20> client_id = get_client_id();
  inline constexpr std::array <std::uint8_t, 8> reserved_bytes {0};
  inline constexpr std::size_t healthy_peer_count = 50;
  inline constexpr std::size_t tcp_bufexp = 16; // 2^tcp_bufexp: if 16 65,636 bytes as buffer size
  inline constexpr std::size_t hanshake_bufexp = 7; // 128 bytes.
  inline constexpr std::size_t max_inbound_inflight = 35;
  inline constexpr std::size_t max_outbound_inflight = 15;
  inline constexpr std::size_t connection_backlog = 128;
  inline constexpr std::size_t rankify_duration = 10; //secs

  // minimum piece size in bittorrent is 16kib, to define what this implememtations
  // largest servicable piece size will be, which determines things like:
  // 1. max outstanding piece storage slots per torrent
  // 2. max outstanding piece log size per peer
  // the largest serviceable piece is is derived via:
  //  2^(max_piece_size_exponent), value should not be less than 14,
  // at 14 this property evaluates to 16kib, the minimum piece length according to the
  // bittorrent spec
  //
  // This value should not go below min_piece_size_exponent
  //
  inline constexpr std::size_t max_piece_size_exponent = 24;

  // This value should never be altered.
  inline constexpr std::size_t min_piece_size_exponent = 14;

  consteval std::size_t max_block_limit () {
    return std::uint64_t{1} << (max_piece_size_exponent - min_piece_size_exponent);
  }

  namespace peer {
    constexpr std::size_t max_reties = 3;
    constexpr std::size_t connect_timeout = 10;//seconds;
    constexpr std::size_t retry_timeout = 15;//seconds;
  }

  namespace file {

  };

}
