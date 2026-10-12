#pragma once
#include "TorrentFile.hpp"
#include "ThreadMessageTypes.hpp"
#include <filesystem>
#include <vector>

class piece_mapper_t;

struct file_piece_t {
  std::size_t file;
  std::size_t offset;
  std::size_t size;
};

struct download_piece_t {
  std::size_t index;
  std::vector<std::byte> piece;
};

struct upload_piece_t {
  std::size_t index;
  std::size_t offset;
  std::vector<std::byte> block; // block.size() should be the logical size of block
};

struct file_range {
  std::size_t start_byte;
  std::size_t end_byte;
};

using incoming_pieces_t=beamable_spsc_t<download_piece_t, 50>;

using outgoing_pieces_t=beamable_spsc_t<upload_piece_t, 50>;

class file_mapper_t {

  std::filesystem::path root_directory;
  std::vector<int> f_descriptors;

public:

  file_mapper_t(std::filesystem::path directory, TorrentFile& torrent);

  // empty files dont need a file descriptor but need to be allocated for
  // file index compliance with piece mapper file indexing hence why ::get_handle_for
  // returns an oprional<int>

  std::optional<int> get_handle_for(std::size_t file_idx) const;

private:

  friend piece_mapper_t;

  std::optional<std::filesystem::path> if_valid_get_path (const ben::decoded_type::list& path_list) const;

  bool is_valid_name(const std::string& name) const;

  bool preallocate(const std::filesystem::path&, file_range& , std::size_t file_idx);

};

class piece_mapper_t {

  const std::size_t piece_length;
  std::vector<file_range> file_ranges;

public:

  piece_mapper_t( TorrentFile&, file_mapper_t& );
  std::vector<file_piece_t> files_for(std::size_t piece_idx) const;

};

struct FileManager {

  file_mapper_t file_map;
  piece_mapper_t piece_map;

  FileManager(std::filesystem::path path, TorrentFile& torrent):
    file_map(path, torrent), piece_map(torrent, file_map)
  {}

};
