// apex.hh
//
// Copyright 2025 Eric Smith
// SPDX-License-Identifier: GPL-3.0-only

#ifndef APEX_HH
#define APEX_HH

#include <iterator>
#include <string>

#include <magic_enum.hpp>
#include <magic_enum_containers.hpp>

#include "apple_ii_disk_image.hh"

namespace apex
{
  static constexpr std::size_t BYTES_PER_BLOCK = 256;
  static constexpr std::size_t BLOCKS_PER_DIRECTORY = 4;
  static constexpr std::size_t ENTRIES_PER_DIRECTORY = 48;
  static constexpr std::size_t DIRECTORIES_PER_DISK = 2;

  class Disk;
  class Directory;

  struct DateError: public std::runtime_error
  { DateError(const std::string& what); };

  class Date
  {
  public:
    static constexpr int EPOCH_YEAR = 1976;

    Date(std::uint16_t raw);
    Date(unsigned year,
	 std::uint8_t month,
	 std::uint8_t day);

    std::uint16_t get_year() const;
    std::uint8_t get_month() const;
    std::uint8_t get_day() const;

    std::string to_string() const;

  private:
    std::uint16_t m_raw;
  };

  class DirectoryEntry
  {
  public:
    static constexpr unsigned FILENAME_CHARS = 8;
    static constexpr unsigned EXTENSION_CHARS = 3;

    enum DirectoryOffset
    {
      // starting offset of per-file fields, indexed by directory entry number
      FILENAME = 0,                              // 11 bytes
      STATUS = (FILENAME_CHARS + EXTENSION_CHARS) * ENTRIES_PER_DIRECTORY,  //  1 byte
      FIRST_BLOCK = 12 * ENTRIES_PER_DIRECTORY,  //  2 bytes
      LAST_BLOCK = 14 * ENTRIES_PER_DIRECTORY,   //  2 bytes
      FDATE = 0x398,                             //  2 bytes

      // offset of per-volume fields
      PRDEV = 0x34a,
      PMAXB = 0x34b,
      PRNAME = 0x34d,
      TITLE = 0x358,
      VOLUE = 0x394,
      DIRDAT = 0x396,
      FLAGS = 0x3f8,
    };

    enum Status: std::uint8_t
    {
      INVALID = 0,
      VALID = 1,
      REPLACE = 254,
      TENTATIVE = 255,
    };

    void replace(Status status,
		 const std::string& filename,
		 std::uint16_t first_block,
		 std::uint16_t last_block,
		 Date date);

    Status get_status() const;
    std::string get_filename() const;
    std::uint16_t get_first_block() const;
    std::uint16_t get_last_block() const;
    Date get_date() const;

  private:
    DirectoryEntry(Directory& dir,
		   std::size_t index);
    Directory& m_dir;
    std::size_t m_index;

    friend class Directory;
  };

  class Directory
  {
  public:
    class iterator
    {
    public:
      using iterator_category = std::bidirectional_iterator_tag;
      using value_type = DirectoryEntry;
      using pointer = const value_type*;
      using reference = const value_type&;

      reference operator*();
      pointer operator->();

      iterator& operator++();    // pre-increment
      iterator operator++(int);  // post-increment

      iterator& operator--();    // pre-decrement
      iterator operator--(int);  // post-decrement

      bool operator==(const iterator& other);
      bool operator!=(const iterator& other);

    private:
      iterator(Directory& dir, std::size_t index);
      Directory& m_dir;
      std::size_t m_index;

      friend class Directory;
    };

    ~Directory();

    iterator begin();
    iterator end();

  private:
    Directory(Disk& disk, std::uint16_t start_block);
    Disk& m_disk;
    std::array<std::uint8_t, BLOCKS_PER_DIRECTORY * BYTES_PER_BLOCK> m_directory_data;
    std::array<DirectoryEntry*, ENTRIES_PER_DIRECTORY> m_directory_entries;

    friend class DirectoryEntry;
    friend class Disk;
  };

  class Disk: public AppleIIDiskImage
  {
  public:
    enum class DirectoryType
    {
      PRIMARY,
      BACKUP,
    };

    static constexpr magic_enum::containers::array<DirectoryType, std::uint16_t> directory_start_block
    {
      9,  // PRIMARY
      13, // BACKUP
    };

    Disk();

    Directory get_directory(DirectoryType type);

  private:
    friend class DirectoryEntry;
    friend class Directory;
  };
} // end namespace apex

#endif // APEX_HH
