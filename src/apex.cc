// apex.cc
//
// Copyright 2025 Eric Smith
// SPDX-License-Identifier: GPL-3.0-only

#include <format>
#include <iostream>

#include "apex.hh"

namespace apex
{
  DateError::DateError(const std::string& what):
    std::runtime_error("Date error: " + what)
  {
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

  void DirectoryEntry::replace(Status status,
			       const std::string& filename,
			       std::uint16_t first_block,
			       std::uint16_t last_block,
			       Date date)
  {
    throw std::runtime_error("DirectoryEntry::set() unimplemented");
    (void) status;
    (void) filename;
    (void) first_block;
    (void) last_block;
    (void) date;
  }

  DirectoryEntry::Status DirectoryEntry::get_status() const
  {
    return static_cast<Status>(m_dir.m_directory_data[DirectoryOffset::STATUS + m_index]);
  }

  static std::string extract_filename_part(const std::uint8_t *data, unsigned count)
  {
    std::string s(reinterpret_cast<const char*>(data), count);
    while (s.size() && (s.back() == ' '))
    {
      s.pop_back();
    }
    return s;
  }

  std::string DirectoryEntry::get_filename() const
  {
    std::size_t filename_offset = DirectoryOffset::FILENAME + m_index * (FILENAME_CHARS + EXTENSION_CHARS);
    std::size_t extension_offset = filename_offset + FILENAME_CHARS;
    std::string fn = extract_filename_part(m_dir.m_directory_data.data() + filename_offset, FILENAME_CHARS);
    std::string ext = extract_filename_part(m_dir.m_directory_data.data() + extension_offset, EXTENSION_CHARS);
    return (ext.size() == 0) ? fn : fn + '.' + ext;
  }

  std::uint16_t DirectoryEntry::get_first_block() const
  {
    std::size_t offset = DirectoryOffset::FIRST_BLOCK + m_index * 2;
    return m_dir.m_directory_data[offset] + (m_dir.m_directory_data[offset + 1] << 8);
  }

  std::uint16_t DirectoryEntry::get_last_block() const
  {
    std::size_t offset = DirectoryOffset::LAST_BLOCK + m_index * 2;
    return m_dir.m_directory_data[offset] + (m_dir.m_directory_data[offset + 1] << 8);
  }

  Date DirectoryEntry::get_date() const
  {
    std::size_t offset = DirectoryOffset::FDATE + m_index * 2;
    std::uint16_t raw = m_dir.m_directory_data[offset] + (m_dir.m_directory_data[offset + 1] << 8);
    return Date(raw);
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
    m_disk(disk)
  {
    disk.read(start_block / AppleIIDiskImage::SECTORS_PER_TRACK,
	      start_block % AppleIIDiskImage::SECTORS_PER_TRACK,
	      BLOCKS_PER_DIRECTORY,
	      m_directory_data.data());
    for (unsigned i = 0; i < ENTRIES_PER_DIRECTORY; i++)
    {
      m_directory_entries[i] = new DirectoryEntry(*this, i);
    }
  }

  Directory::~Directory()
  {
    for (unsigned i = 0; i < ENTRIES_PER_DIRECTORY; i++)
    {
      delete m_directory_entries[i];
    }
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

} // end namespace apex

