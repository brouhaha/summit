// apex.hh
//
// Copyright 2025 Eric Smith
// SPDX-License-Identifier: GPL-3.0-only

#ifndef APEX_HH
#define APEX_HH

#include <iterator>
#include <string>

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

    std::uint16_t get_year();
    std::uint8_t get_month();
    std::uint8_t get_day();

  private:
    std::uint16_t m_raw;
  };

  class DirectoryEntry
  {
  public:
    static constexpr unsigned FILENAME_CHARS = 8;
    static constexpr unsigned EXTENSION_CHARS = 8;

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

    Status get_status();
    std::string get_filename();
    std::uint16_t get_first_block();
    std::uint16_t get_last_block();
    Date get_date();

  private:
    DirectoryEntry(Directory& dir,
		   std::size_t index);
    Directory& m_dir;
    std::size_t m_index;
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

      reference operator*() const;
      pointer operator->() const;

      iterator& operator++();
      iterator operator++(int);

      iterator& operator--();
      iterator operator--(int);

      bool operator==(const iterator& other);
      bool operator!=(const iterator& other);

    private:
      iterator(Directory& dir, int i);
      Directory& m_dir;
      int m_i;

      friend class Directory;
    };

    iterator begin();
    iterator end();

  private:
    Directory(Disk& disk, std::uint16_t start_block);
    Disk& m_disk;
    std::array<std::uint8_t, BLOCKS_PER_DIRECTORY * BYTES_PER_BLOCK> m_directory_data;

    friend class DirectoryEntry;
    friend class Disk;
  };

  class Disk: public AppleIIDiskImage
  {
  public:
    static constexpr std::array<std::uint16_t, DIRECTORIES_PER_DISK> directory_start_block
    {
      9,  // primary directory
      13, // backup directory
    };

    Disk();

    Directory get_directory(unsigned index);

  private:
    friend class DirectoryEntry;
    friend class Directory;
  };
} // end namespace apex

#endif // APEX_HH
