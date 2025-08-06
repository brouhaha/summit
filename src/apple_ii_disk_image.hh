// apple_ii_disk_image.hh
//
// Copyright 2025 Eric Smith
// SPDX-License-Identifier: GPL-3.0-only

#ifndef APPLE_II_DISK_IMAGE_HH
#define APPLE_II_DISK_IMAGE_HH

#include <cstdint>
#include <filesystem>
#include <stdexcept>
#include <vector>

#include <magic_enum.hpp>
#include <magic_enum_containers.hpp>

struct AppleIIDiskImageError: public std::runtime_error
{ AppleIIDiskImageError(const std::string& what); };

class AppleIIDiskImage
{
public:
  enum class ImageFormat
  {
    RAW_ORDER,     // no sector interleave
    DOS_ORDER,     // mostly 2:1 sector interleave, in descending order
    PRODOS_ORDER,  // 2:1 sector interleave
    CPM_ORDER,     // 3:1 sector interleave
  };

  static constexpr std::size_t BYTES_PER_SECTOR = 256;
  static constexpr std::size_t SECTORS_PER_TRACK = 16;
  static constexpr std::size_t TRACKS_PER_DISK = 35;
  static constexpr std::size_t SECTORS_PER_DISK = TRACKS_PER_DISK * SECTORS_PER_TRACK;
  static constexpr std::size_t BYTES_PER_DISK = SECTORS_PER_DISK * BYTES_PER_SECTOR;
  
  AppleIIDiskImage();

  static bool validate_sector_maps();

  void load(ImageFormat format,
	    const std::filesystem::path& filename);
  void save(ImageFormat format,
	    const std::filesystem::path& filename);

  void read(std::uint8_t track,
	    std::uint8_t sector,
	    std::size_t sector_count,
	    std::uint8_t* data);

  void write(std::uint8_t track,
	     std::uint8_t sector,
	     std::size_t sector_count,
	     const std::uint8_t* data);

protected:
  std::size_t m_sector_count;
  std::vector<std::uint8_t> m_image;

  using SectorMap = std::array<std::uint8_t, SECTORS_PER_TRACK>;

  static constexpr magic_enum::containers::array<ImageFormat, SectorMap> logical_to_physical_sector_map
  {
    /* RAW_ORDER */    SectorMap { 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7,
				   0x8, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf },
    /* DOS_ORDER */    SectorMap { 0x0, 0xd, 0xb, 0x9, 0x7, 0x5, 0x3, 0x1,
				   0xe, 0xc, 0xa, 0x8, 0x6, 0x4, 0x2, 0xf },
    /* PRODOS_ORDER */ SectorMap { 0x0, 0x2, 0x4, 0x6, 0x8, 0xa, 0xc, 0xe,
				   0x1, 0x3, 0x5, 0x7, 0x9, 0xb, 0xd, 0xf },
    /* CPM_ORDER */    SectorMap { 0x0, 0x3, 0x6, 0x9, 0xc, 0xf, 0x2, 0x5,
				   0x8, 0xb, 0xe, 0x1, 0x4, 0x7, 0xa, 0xd },
  };

  static constexpr magic_enum::containers::array<ImageFormat, SectorMap> physical_to_logical_sector_map
  {
    /* RAW_ORDER */    SectorMap { 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7,
				   0x8, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf },
    /* DOS_ORDER */    SectorMap { 0x0, 0x7, 0xe, 0x6, 0xd, 0x5, 0xc, 0x4,
				   0xb, 0x3, 0xa, 0x2, 0x9, 0x1, 0x8, 0xf },
    /* PRODOS_ORDER */ SectorMap { 0x0, 0x8, 0x1, 0x9, 0x2, 0xa, 0x3, 0xb,
				   0x4, 0xc, 0x5, 0xd, 0x6, 0xe, 0x7, 0xf },
    /* CPM_ORDER */    SectorMap { 0x0, 0xb, 0x6, 0x1, 0xc, 0x7, 0x2, 0xd,
				   0x8, 0x3, 0xe, 0x9, 0x4, 0xf, 0xa, 0x5 },
  };
};

#endif // APPLE_II_DISK_IMAGE_HH
