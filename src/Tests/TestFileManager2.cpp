#include "../Client/TorrentFile.hpp"
#include "../Client/FileManager.hpp"
#include <ios>
#include "../Client/Hasher.hpp"

int main(int argc, char* argv[]) {

  if (argc != 3) { std::cerr << "give a metainfo file and download path\n"; return 1; }

  TorrentFile tfile {argv[1]};
  FileManager disk  (argv[2], tfile);

  std::cout << std::boolalpha;

  for ( std::size_t i = 0; i < tfile.get_file_count(); ++i ) {

    auto& file = disk.file_map.get_file_at(i);

    std::cout << "Path:\t\t" << file.path << '\n';
    std::cout << "Desrciptor:\t" << file.descriptor << '\n';
    std::cout << "Size:\t\t" << file.size << '\n';
    std::cout << "created:\t" << file.created << '\n';

    std::cout << '\n';

  }


  std::cout << "Torrent Hash:\t\t" << Hasher::hex_stringify_hash(tfile.get_info_hash()) << '\n';
  std::cout << "Torrent Name:\t\t" << tfile.get_torrent_name() << '\n';
  std::cout << "Torrent Size:\t\t" << tfile.get_download_size() << '\n';
  std::cout << "Torrent Piece length:\t" << tfile.get_piece_length() << '\n';
  std::cout << "Torrent Piece Hash[1]:\t" << tfile.get_piece_hash(0) << '\n';
  std::cout << "Torrent File?:\t\t" << tfile.torrent_is_file() << '\n';
  std::cout << "Torrent Piece Count:\t" << tfile.get_piece_count() << '\n';
  std::cout << "Torrent files:\t\t" << tfile.get_file_count() << '\n';

}
