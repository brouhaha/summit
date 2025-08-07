// apex.hh
//
// Copyright 2025 Eric Smith
// SPDX-License-Identifier: GPL-3.0-only

#ifndef APEX_HH
#define APEX_HH

#include <array>
#include <iterator>
#include <string>
#include <vector>

#include <boost/dynamic_bitset.hpp>

#include <magic_enum.hpp>
#include <magic_enum_containers.hpp>

#include "apple_ii_disk_image.hh"

namespace Apex
{
  class Disk;
  class Directory;

  static constexpr std::size_t BYTES_PER_BLOCK = 256;
  static constexpr std::size_t BLOCKS_PER_DIRECTORY = 4;
  static constexpr std::size_t ENTRIES_PER_DIRECTORY = 48;
  static constexpr std::size_t DIRECTORIES_PER_DISK = 2;

  static constexpr unsigned FILENAME_CHARS = 8;
  static constexpr unsigned EXTENSION_CHARS = 3;

  struct FilenameError: std::runtime_error
  { FilenameError(const std::string& what); };

  class Filename
  {
  public:
    std::vector<char> name;
    std::vector<char> ext;

    // invalid filename
    Filename();

    Filename(const std::string& pattern);

    // raw Apex filename, must be exactly 11 characters,
    // name and extension padded with spaces, no period
    // separator
    Filename(const char* data, std::size_t length);

    bool has_wildcard() const;

    bool match(const Filename& other) const;

    std::string to_string() const;

  private:
    bool m_has_wildcard;
  };

  struct BlockRange
  {
    std::uint16_t begin;
    std::uint16_t end;  // plus one
  };

  enum class DiskArea
  {
    BOOT,
    PRIMARY_DIRECTORY,
    BACKUP_DIRECTORY,
    FILE_AREA,
  };

  static constexpr magic_enum::containers::array<DiskArea, BlockRange> disk_area_block_range
  {
    /* BOOT              */ BlockRange {   0,   9 },
    /* PRIMARY_DIRECTORY */ BlockRange {   9,  13 },
    /* BACKUP_DIRECTORY  */ BlockRange {  13,  17 },
    /* FILE_AREA         */ BlockRange {  17, 560 },
  };

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

  enum DirectoryOffset
  {
    // starting offset of per-file fields, indexed by directory entry number
    FILENAME = 0,                              // 11 bytes
    STATUS = (FILENAME_CHARS + EXTENSION_CHARS) * ENTRIES_PER_DIRECTORY,  //  1 byte
    FIRST_BLOCK = 12 * ENTRIES_PER_DIRECTORY,  //  2 bytes
    LAST_BLOCK = 14 * ENTRIES_PER_DIRECTORY,   //  2 bytes

    // 74 bytes unused from 0x300..0x349

    // offset of per-volume fields
    PRDEV = 0x34a,  // 1 byte
    PMAXB = 0x34b,  // 2 bytes - max block - unused - 0x01c6 (456), should be 0x230 (560)
    PRNAME = 0x34d, // 11 bytes
    TITLE = 0x358,  // 32 bytes

    // 28 bytes unused from 0x378..0x393

    VOLUME = 0x394, // 2 bytes
    DIRDAT = 0x396, // 2 bytes

    // another per-file field, indexed by directory entry number
    FDATE = 0x398,                             //  2 bytes

    // more per-volume fields
    FLAGS = 0x3f8,  // 8 bytes - unused
  };

  class DirectoryEntry
  {
  public:
    enum Status: std::uint8_t
    {
      INVALID     = 0x00,
      VALID       = 0x01,
      DISK_ERASED = 0xe5,
      REPLACE     = 0xfe,
      TENTATIVE   = 0xff,
    };

    void delete_file();

    void replace(Status status,
		 const Filename& filename,
		 std::uint16_t first_block,
		 std::uint16_t last_block,
		 Date date);

    Status get_status() const;
    Filename get_filename() const;
    std::uint16_t get_first_block() const;
    std::uint16_t get_last_block() const;
    std::uint16_t get_block_count() const;
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
      using pointer = value_type*;
      using reference = value_type&;

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

    std::size_t volume_size_blocks() const;
    std::size_t volume_free_blocks() const;

    // returns 0 if not found
    std::uint16_t find_free_blocks(std::uint16_t requested_block_count) const;

    void debug_list_free_blocks() const;

    iterator begin();
    iterator end();

  private:
    Directory(Disk& disk, std::uint16_t start_block);
    void update_free_bitmap();
    void update_disk_image();

    Disk& m_disk;
    std::uint16_t m_start_block;

    std::array<std::uint8_t, BLOCKS_PER_DIRECTORY * BYTES_PER_BLOCK> m_directory_data;
    std::array<DirectoryEntry*, ENTRIES_PER_DIRECTORY> m_directory_entries;
    boost::dynamic_bitset<> m_free_bitmap;

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

    void read(std::uint16_t block_number,
	      std::size_t block_count,
	      std::uint8_t* data);

    void write(std::uint16_t block_number,
	       std::size_t block_count,
	       const std::uint8_t* data);

  private:
    friend class DirectoryEntry;
    friend class Directory;
  };
} // end namespace Apex

#endif // APEX_HH
