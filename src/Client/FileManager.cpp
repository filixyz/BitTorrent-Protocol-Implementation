#include "FileManager.hpp"
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <optional>
#include <fcntl.h>
#include <span>
#include <stdexcept>
#include <linux/limits.h>
#include <string_view>
#include <system_error>

file_mapper_t::file_mapper_t(std::filesystem::path directory, TorrentFile& torrent) {

  file_table.reserve(torrent.get_file_count());

  std::error_code err;
  root_directory = std::filesystem::weakly_canonical(directory, err);

  if (err) throw std::runtime_error{"Cannot resolve download directory"};

  if ( const std::string torrent_directory {torrent.get_torrent_name()}; !torrent.torrent_is_file() ) {

    if (!is_valid_name(torrent_directory)) throw std::runtime_error{"Invalid Torrent Name"};

    root_directory /= torrent_directory;

    if (std::filesystem::is_symlink(root_directory)) throw std::runtime_error{"Torrent directory is a symlink"};

  }

  std::filesystem::create_directories(root_directory);

}

void file_mapper_t::index_file(std::span<const std::string_view> path_names, std::size_t size) {

  const auto path = if_valid_get_path(path_names);

  if (!path) throw std::runtime_error{"Invalid file path from torrent"};

  file_table.emplace_back( path.value(), size );

}

bool file_mapper_t::is_valid_name(const std::string_view name) const {

  if  (name.empty())                          return false;
  if  (name.length() > NAME_MAX)              return false;
  if  (name == "." or name == "..")           return false;
  if  (name.find('/')  != std::string::npos)  return false;
  if  (name.find('\\') != std::string::npos)  return false;
  if  (name.find('\0') != std::string::npos)  return false;

  return true;

}

std::optional<std::filesystem::path> file_mapper_t::if_valid_get_path (std::span<const std::string_view> path) const {

  // very naive path validator

  if (path.empty()) return std::nullopt;

  std::filesystem::path file_path = root_directory;

  for (const auto& name : path ) {

    if ( !is_valid_name(name) ) return std::nullopt;

    file_path /= name;

  }

  file_path = file_path.lexically_normal();

  std::error_code error;
  file_path = std::filesystem::weakly_canonical(file_path, error);

  if (error) return std::nullopt;

  if (file_path.string().length() >= PATH_MAX) return std::nullopt;

  auto relative_path = file_path.lexically_relative(root_directory);

  if (

    relative_path.empty()           or
    relative_path == "."            or
    *relative_path.begin() == ".."

  ) return std::nullopt;

  return file_path;

}

bool file_mapper_t::preallocate(file_t& file) {

  std::filesystem::create_directories(file.path.parent_path());

  // allocate here.

  return true;

}

piece_mapper_t::piece_mapper_t(TorrentFile& torrent, file_mapper_t& file_mapper)
  : piece_length(torrent.get_piece_length()) , file_ranges(torrent.get_file_count())
{

  if (torrent.torrent_is_file()) {

    file_ranges[0] = { 0, static_cast<std::size_t>( torrent.get_download_size() ) };

    const auto name = torrent.get_torrent_name();

    file_mapper.index_file(std::span{&name, 1}, torrent.get_download_size());

    return;

  }

  const auto& files = torrent.files().value().get();

  std::size_t start_byte = 0;

  for (std::size_t file_index = 0; const auto& file : files) {

    const auto& file_map =
      file
      .get_as<ben::decoded_type::dictionary>();

    const std::size_t file_size =
      (*file_map.find("length"))
      .second
      .get_as<ben::decoded_type::integer>();

    file_ranges[file_index] =  {start_byte, file_size + start_byte};

    const auto& path_list =
      (*file_map.find("path"))
      .second
      .get_as<ben::decoded_type::list>();

    std::vector<std::string_view> decoded_path_list;

    for (const auto& path : path_list) {

      if (path.type() != ben::encode_type::string) throw std::runtime_error{" torrent path list names not string "};

      decoded_path_list.push_back(path.get_as<ben::decoded_type::string>());

    }

    file_mapper.index_file( decoded_path_list, file_size );

    start_byte += file_size;

    ++file_index;
  }

}

std::vector<file_piece_t> piece_mapper_t::files_for(std::size_t piece_index) const {

  static constexpr auto comparator = [](const file_range& a, std::size_t b){ return a.end_byte < b; };

  const std::size_t piece_byte_mark = piece_index * piece_length;

  auto found = std::lower_bound(file_ranges.begin(), file_ranges.end(), piece_byte_mark, comparator);

  std::size_t file_idx = found - file_ranges.begin();

  std::vector<file_piece_t> files;

  std::size_t piece_remaining = piece_length;

  for (; file_idx < file_ranges.size() && piece_remaining != 0; ++file_idx) {

    const auto& file_map  = file_ranges[file_idx];

    std::size_t file_start = 0;
    std::size_t available_in_file = file_map.end_byte - file_map.start_byte;

    // skip ghost files with file sizes equal to zero
    // and files where the piece byte mark is exactly where the current file
    // ended meaning this piece contributes nothing to it.

    if (!available_in_file or piece_byte_mark == file_map.end_byte) continue;

    if ( piece_byte_mark > file_map.start_byte ) {
      file_start = piece_byte_mark - file_map.start_byte;
      available_in_file = file_map.end_byte - piece_byte_mark;
    }

    std::size_t writable = std::min( available_in_file, piece_remaining );

    files.push_back({ file_idx, file_start, writable });

    piece_remaining -= writable;

  }

  return files;
}

file_t& file_mapper_t::get_file_at(std::size_t file_idx) {

  assert(file_idx < file_table.size());

  return file_table[file_idx];

}
