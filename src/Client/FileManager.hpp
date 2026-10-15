#pragma once
#include "TorrentFile.hpp"
#include "ThreadMessageTypes.hpp"
#include <filesystem>
#include <vector>

struct file_t {

  bool created {false};
  int descriptor  {-1}; // -1 = no fd (not opened yet, or zero-size file)
  const std::size_t size {0};
  const std::filesystem::path path;

  file_t(const std::filesystem::path _path, std::size_t _size)
    : size(_size), path(_path)
  {}

};

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
  std::vector<file_t> file_table;

private:

  bool is_valid_name(const std::string_view name) const;

  bool preallocate(file_t&);

  std::optional<std::filesystem::path> if_valid_get_path (std::span<const std::string_view> path) const;

public:

  file_mapper_t(std::filesystem::path directory, TorrentFile& torrent);

  file_t& get_file_at(std::size_t file_idx);

  // Should only be called in piece mappers constructor will for file mapper file indexing

  void index_file(std::span<const std::string_view> path_names, std::size_t size);

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
