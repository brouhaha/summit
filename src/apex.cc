// apex.cc
//
// Copyright 2025 Eric Smith
// SPDX-License-Identifier: GPL-3.0-only

#include <algorithm>
#include <chrono>
#include <cstring>
#include <format>
#include <iostream>

#include "apex.hh"
#include "utility.hh"

namespace Apex
{
  FilenameError::FilenameError(const std::string& what):
    std::runtime_error("Filename error: " + what)
  {
  }

  Filename::Filename():
    name(FILENAME_CHARS, ' '),
    ext(EXTENSION_CHARS, ' '),
    m_has_wildcard(false)
  {
  }

  Filename::Filename(const std::string& pattern):
    name(FILENAME_CHARS, ' '),
    ext(EXTENSION_CHARS, ' '),
    m_has_wildcard(false)
  {
    std::vector<char>* current_part = &name;
    unsigned index = 0;
    bool have_star = false;
    for (const char c: pattern)
    {
      if (((c >= 'A') && (c <= 'Z')) ||
	  ((c >= 'a') && (c <= 'z')) ||
	  ((c >= '0') && (c <= '9') && (index != 0)) ||
	  (c == '?') ||
	  (c == '*'))
      {
	if (index >= current_part->size())
	{
	  throw FilenameError("filename component too long");
	}
	if (have_star)
	{
	  throw FilenameError("filename component has characters after star");
	}
	(*current_part)[index++] = c;
	m_has_wildcard |= ((c == '?') || (c == '*'));
	have_star = (c == '*');
      }
      else if (c == '.')
      {
	if (current_part == &name)
	{
	  current_part = &ext;
	  index = 0;
	  have_star = false;
	}
	else
	{
	  throw FilenameError("can only have one extension");
	}
      }
      else
      {
	throw FilenameError(std::format("charater '{}' not allowed in filespec", c));
      }
    }
  }

  Filename::Filename(const char* data, std::size_t length):
    name(FILENAME_CHARS),
    ext(EXTENSION_CHARS),
    m_has_wildcard(false)
  {
    if (length != (FILENAME_CHARS + EXTENSION_CHARS))
    {
      throw FilenameError(std::format("raw Apex filespec must be exactly {} characters", FILENAME_CHARS + EXTENSION_CHARS));
    }
    std::memcpy(name.data(), data, FILENAME_CHARS);
    std::memcpy(ext.data(),  data + FILENAME_CHARS, EXTENSION_CHARS);
  }

  bool Filename::has_wildcard() const
  {
    return m_has_wildcard;
  }

  static bool part_match(const std::vector<char>& pat,
			 const std::vector<char>& fn)
  {
    for (unsigned i = 0; i < pat.size(); i++)
    {
      if (pat[i] == '*')
      {
	return true;  // wildcard match entire remainder
      }
      if (pat[i] == '?')
      {
	continue;  // wildcard match one character position
      }
      if (pat[i] == ' ')
      {
	return fn[i] == ' '; // succeed, matched up to trailing space fill
      }
      if (utility::upcase_character(pat[i]) != utility::upcase_character(fn[i]))
      {
	return false;  // fail due to character mismatch
      }
    }
    return true;  // succeed, all matched
  }

  bool Filename::match(const Filename& other) const
  {
    return part_match(name, other.name) && part_match(ext, other.ext);
  }

  static std::string part_to_string(const std::vector<char>& part)
  {
    std::size_t length = part.size();
    while (length && (part[length - 1] == ' '))
    {
      --length;
    }
    return std::string(part.begin(), part.begin() + length);
  }

  std::string Filename::to_string() const
  {
    std::string n = part_to_string(name);
    std::string e = part_to_string(ext);
    return e.size() ? n + '.' + e : n;
  }

  Filename Filename::upcase() const
  {
    Filename fn;
    for (std::size_t i = 0; i < FILENAME_CHARS; i++)
    {
      fn.name[i] = utility::upcase_character(name[i]);
    }
    for (std::size_t i = 0; i < EXTENSION_CHARS; i++)
    {
      fn.ext[i] = utility::upcase_character(ext[i]);
    }

    return fn;
  }


  DateError::DateError(const std::string& what):
    std::runtime_error("Date error: " + what)
  {
  }

  Date::Date()
  {
    const auto now = std::chrono::system_clock::now();
    time_t tt = std::chrono::system_clock::to_time_t(now);
    struct tm local_tm = *localtime(&tt);
    m_raw = (((local_tm.tm_year + 1900 - EPOCH_YEAR) << 9) +
	     ((local_tm.tm_mon + 1) << 5) +
	     local_tm.tm_mday);
  }

  Date::Date(std::uint16_t raw):
    m_raw(raw)
  {
  }
  
  Date::Date(unsigned year,
	     std::uint8_t month,
	     std::uint8_t day)
  {
    if ((year < EPOCH_YEAR) || (year > (EPOCH_YEAR + 127)))
    {
      throw DateError(std::format("Date: invalid year {}\n", year));
    }
    if ((month < 1) || (month > 12))
    {
      throw DateError(std::format("Date: invalid month {}\n", year));
    }
    if ((day < 1) || (day > 31))
    {
      throw DateError(std::format("Date: invalid month {}\n", year));
    }
    m_raw = ((year - EPOCH_YEAR) << 9) + (month << 5) + day;
  }

  std::uint16_t Date::get_year() const
  {
    return (m_raw >> 9) + EPOCH_YEAR;
  }
  
  std::uint8_t Date::get_month() const
  {
    return (m_raw >> 5) & 0xf;
  }
  
  std::uint8_t Date::get_day() const
  {
    return m_raw & 0x1f;
  }

  std::uint16_t Date::get_raw() const
  {
    return m_raw;
  }
  
  std::string Date::to_string() const
  {
    return std::format("{:04d}-{:02d}-{:02d}", get_year(), get_month(), get_day());
  }


  DirectoryEntry::DirectoryEntry(Directory& dir,
				 std::size_t index):
    m_dir(dir),
    m_index(index)
  {
  }

  void DirectoryEntry::delete_file()
  {
    m_dir.m_directory_data[DirectoryOffset::STATUS + m_index] = static_cast<std::uint8_t>(Status::INVALID);
    m_dir.update_free_bitmap();
    m_dir.update_disk_image();
  }

  void DirectoryEntry::replace(Status status,
			       const Filename& filename,
			       std::uint16_t first_block,
			       std::uint16_t last_block,
			       Date date)
  {
    if (m_dir.m_directory_data[DirectoryOffset::STATUS + m_index] != static_cast<std::uint8_t>(Status::INVALID))
    {
      throw std::runtime_error("can't overwrite a directory entry that is in use");
    }
    
    m_dir.m_directory_data[DirectoryOffset::STATUS + m_index] = static_cast<Status>(status);

    Filename fn = filename.upcase();

    std::size_t filename_offset = DirectoryOffset::FILENAME + m_index * (FILENAME_CHARS + EXTENSION_CHARS);
    std::memcpy(m_dir.m_directory_data.data() + filename_offset,
		fn.name.data(),
		FILENAME_CHARS);
    std::memcpy(m_dir.m_directory_data.data() + filename_offset + FILENAME_CHARS,
		fn.ext.data(),
		EXTENSION_CHARS);

    m_dir.write_u16(DirectoryOffset::FIRST_BLOCK + m_index * 2, first_block);
    m_dir.write_u16(DirectoryOffset::LAST_BLOCK + m_index * 2, last_block);
    m_dir.write_u16(DirectoryOffset::FDATE + m_index * 2, date.get_raw());

    m_dir.update_free_bitmap();
    m_dir.update_disk_image();
  }

  DirectoryEntry::Status DirectoryEntry::get_status() const
  {
    return static_cast<Status>(m_dir.m_directory_data[DirectoryOffset::STATUS + m_index]);
  }

  Filename DirectoryEntry::get_filename() const
  {
    std::size_t filename_offset = DirectoryOffset::FILENAME + m_index * (FILENAME_CHARS + EXTENSION_CHARS);
    Filename f(reinterpret_cast<const char*>(m_dir.m_directory_data.data() + filename_offset), FILENAME_CHARS + EXTENSION_CHARS);
    return f;
  }

  std::uint16_t DirectoryEntry::get_first_block() const
  {
    std::size_t offset = DirectoryOffset::FIRST_BLOCK + m_index * 2;
    return m_dir.read_u16(offset);
  }

  std::uint16_t DirectoryEntry::get_last_block() const
  {
    std::size_t offset = DirectoryOffset::LAST_BLOCK + m_index * 2;
    return m_dir.read_u16(offset);
  }

  std::uint16_t DirectoryEntry::get_block_count() const
  {
    return get_last_block() + 1 - get_first_block();
  }

  Date DirectoryEntry::get_date() const
  {
    std::size_t offset = DirectoryOffset::FDATE + m_index * 2;
    return Date(m_dir.read_u16(offset));
  }

  Directory::iterator::iterator(Directory& dir, std::size_t index):
    m_dir(dir),
    m_index(index)
  {
  }

  Directory::iterator& Directory::iterator::operator++()  // pre-increment
  {
    ++m_index;
    return *this;
  }

  Directory::iterator Directory::iterator::operator++(int)  // post-increment
  {
    iterator old = *this;
    operator++();
    return old;
  }

  Directory::iterator& Directory::iterator::operator--()  // pre-decrement
  {
    --m_index;
    return *this;
  }

  Directory::iterator Directory::iterator::operator--(int)  // post-decrement
  {
    iterator old = *this;
    operator--();
    return old;
  }

  bool Directory::iterator::operator==(const iterator& other)
  {
    bool equal = (& m_dir == & other.m_dir) && (m_index == other.m_index);
    return equal;
  }

  bool Directory::iterator::operator!=(const iterator& other)
  {
    return ! operator==(other);
  }

  Directory::iterator::reference Directory::iterator::operator*()
  {
    if (m_index >= ENTRIES_PER_DIRECTORY)
    {
      throw std::runtime_error(std::format("dereferencing iterator with index {}", m_index));
    }
    return *m_dir.m_directory_entries[m_index];
  }

  Directory::Directory(Disk& disk, std::uint16_t start_block):
    m_disk(disk),
    m_start_block(start_block)
  {
    m_disk.read(m_start_block,
		BLOCKS_PER_DIRECTORY,
		m_directory_data.data());
    update_free_bitmap();
  }

  Directory::~Directory()
  {
    for (unsigned i = 0; i < ENTRIES_PER_DIRECTORY; i++)
    {
      delete m_directory_entries[i];
    }
  }

  std::uint16_t Directory::read_u16(std::size_t offset) const
  {
    return m_directory_data[offset] | (m_directory_data[offset + 1] << 8);
  }

  void Directory::write_u16(std::size_t offset, std::uint16_t value)
  {
    m_directory_data[offset] = value & 0xff;
    m_directory_data[offset + 1] = value >> 8;
  }

  DirectoryEntry& Directory::allocate_directory_entry()
  {
    for (DirectoryEntry* entry: m_directory_entries)
    {
      if (entry->get_status() == DirectoryEntry::Status::INVALID)
      {
	return *entry;
      }
    }
    throw std::runtime_error("out of directory entries");
  }

  void Directory::update_free_bitmap()
  {
    std::uint16_t max_block = volume_size_blocks();
    m_free_bitmap.resize(max_block);
    m_free_bitmap.reset();
    for (std::size_t block = disk_area_block_range[DiskArea::FILE_AREA].begin;
	 block < max_block;
	 ++block)
    {
      m_free_bitmap[block] = true;
    }
    bool consistency_error = false;
    for (unsigned i = 0; i < ENTRIES_PER_DIRECTORY; i++)
    {
      auto entry = new DirectoryEntry(*this, i);
      m_directory_entries[i] = entry;
      if (entry->get_status() == DirectoryEntry::Status::VALID)
      {
	std::uint16_t first = entry->get_first_block();
	std::uint16_t last = entry->get_last_block();
	for (std::size_t block = first; block <= last; block++)
	{
	  if (! m_free_bitmap[block])
	  {
	    consistency_error = true;
	  }
	  m_free_bitmap[block] = false;
	}
      }
    }
    if (consistency_error)
    {
      std::cerr << "directory inconsistent - file block ranges incorrect or overlap\n";
    }
  }

  void Directory::update_disk_image()
  {
    m_disk.write(m_start_block,
		 BLOCKS_PER_DIRECTORY,
		 m_directory_data.data());
  }

  std::size_t Directory::volume_size_blocks() const
  {
    return read_u16(DirectoryOffset::PMAXB) + 1;
  }

  std::size_t Directory::volume_free_blocks() const
  {
    return m_free_bitmap.count();
  }

  std::uint16_t Directory::find_free_blocks(std::uint16_t requested_block_count) const
  {
    std::size_t max_block = volume_size_blocks();
    boost::dynamic_bitset<> m_used_bitmap = ~m_free_bitmap;
    std::size_t free_block_num = m_free_bitmap.find_first();
    while (free_block_num < max_block)
    {
      std::size_t used_block_num = std::min(m_used_bitmap.find_next(free_block_num), max_block);
      std::size_t free_block_count = used_block_num - free_block_num;
      if (free_block_count >= requested_block_count)
      {
	return free_block_num;
      }
      free_block_num = m_free_bitmap.find_next(used_block_num);
    }
    return 0;  // failed to find requested number of free blocks
  }

  void Directory::debug_list_free_blocks() const
  {
    std::size_t max_block = volume_size_blocks();
    boost::dynamic_bitset<> m_used_bitmap = ~m_free_bitmap;
    std::cout << "Free blocks:\n";
    std::size_t free_extent_count = 0;
    std::size_t free_block_count = 0;
    std::size_t free_block_num = m_free_bitmap.find_first();
    while (free_block_num < max_block)
    {
      std::size_t used_block_num = std::min(m_used_bitmap.find_next(free_block_num), max_block);
      std::cout << std::format("{} blocks free from {} through {}\n",
			       used_block_num - free_block_num,
			       free_block_num,
			       used_block_num-1);
      ++free_extent_count;
      free_block_count += used_block_num - free_block_num;
      free_block_num = m_free_bitmap.find_next(used_block_num);
    }
    std::cout << std::format("total {} free blocks found in {} extents\n",
			     free_block_count,
			     free_extent_count);
  }

  Directory::iterator Directory::begin()
  {
    // XXX need to use the first non-deleted directory entry,
    //     of if there isn't one, npos
    return iterator(*this, 0);
  }

  Directory::iterator Directory::end()
  {
    // XXX should use npos
    return iterator(*this, ENTRIES_PER_DIRECTORY);
  }

  Disk::Disk()
  {
  }

  Directory Disk::get_directory(DirectoryType type)
  {
    return Directory(*this, directory_start_block.at(type));
  }

  void Disk::read(std::uint16_t block_number,
		  std::size_t block_count,
		  std::uint8_t* data)
  {
    AppleIIDiskImage::read(block_number / AppleIIDiskImage::SECTORS_PER_TRACK,
			   block_number % AppleIIDiskImage::SECTORS_PER_TRACK,
			   block_count,
			   data);
  }

  void Disk::write(std::uint16_t block_number,
		   std::size_t block_count,
		   const std::uint8_t* data)
  {
    AppleIIDiskImage::write(block_number / AppleIIDiskImage::SECTORS_PER_TRACK,
			    block_number % AppleIIDiskImage::SECTORS_PER_TRACK,
			    block_count,
			    data);
  }

} // end namespace Apex

