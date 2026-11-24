#include "../Client/TransferManagerTypes.hpp"
#include <cstddef>
#include <iostream>
#include "../Client/Randomer.hpp"
int main() {

  piece_picker_t piece_manager{10};

  int_randomer<std::size_t> random {std::size_t(0), std::size_t(9)};

  for (std::size_t i = 20; i != 0; --i) {

    for(const auto& a : piece_manager.ranked_pieces)  std::cout << a << "\t";
    std::cout << '\n';
    for(const auto& a : piece_manager.rank_partition) std::cout << a << "\t";
    std::cout << '\n';
    std::cout << '\n';

    auto rand_piece = random.get();
    piece_manager.increase_piece_rank(rand_piece);

    std::cout << "Piece: " << rand_piece << " Increased Rank\n";
    std::cout << "---------------------------------------\n";

  }

}
