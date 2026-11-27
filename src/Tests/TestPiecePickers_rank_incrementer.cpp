#include "../Client/TransferManagerTypes.hpp"
#include <cmath>
#include <cstddef>
#include <iostream>
#include "../Client/Randomer.hpp"
#include <chrono>
#include <ratio>

int main() {

  piece_picker_t piece_manager{10};

  int_randomer<std::size_t> random {std::size_t(0), std::size_t(9)};

  std::chrono::time_point start = std::chrono::steady_clock::now();

  std::chrono::nanoseconds dur;

  for (std::size_t i = 1000; i != 0; --i) {

    for(const auto& a : piece_manager.ranked_pieces)  std::cout << a <<" : " << piece_manager.get_rank( piece_manager.piece_map[a] ) << "\t";
    std::cout << '\n';
    for(const auto& a : piece_manager.rank_partition) std::cout << a << "\t";
    std::cout << '\n';
    std::cout << '\n';

    auto rand_piece = random.get();

    std::chrono::time_point start__ = std::chrono::steady_clock::now();

    piece_manager.increase_piece_rank(rand_piece);

    std::chrono::time_point end__ = std::chrono::steady_clock::now();

    dur = end__ - start__;

    std::cout << "Piece: " << rand_piece << " Increased Rank\n";
    std::cout << "---------------------------------------\n";

  }

  std::chrono::time_point end = std::chrono::steady_clock::now();

  auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end -start);

  std::cout << "Time elasped for 1000 random piece increments among 10 pieces and table adjust " << elapsed << " \n";
  std::cout << "duration per piece is: " << elapsed/1000 << "\n";

  std::cout << "duration for last invoke of piece incremneter with no noise: " << dur << '\n';

}
