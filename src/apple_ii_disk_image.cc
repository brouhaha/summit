// apple_ii_disk_image.cc
//
// Copyright 2025 Eric Smith
// SPDX-License-Identifier: GPL-3.0-only

#include <cstring>
#include <format>
#include <fstream>
#include <iostream>

#include <magic_enum_utility.hpp>

#include "apple_ii_disk_image.hh"

AppleIIDiskImageError::AppleIIDiskImageError(const std::string& what):
  std::runtime_error("Apple II disk image error: " + what)
{
}

AppleIIDiskImage::AppleIIDiskImage():
  m_sector_count(SECTORS_PER_DISK),
  m_image(BYTES_PER_DISK)
{
}

bool AppleIIDiskImage::validate_sector_maps()
{
  unsigned error_count = 0;
  std::cout << "validating sector interleave maps\n";
  magic_enum::enum_for_each<ImageFormat>([&error_count] (ImageFormat format)
      {
	for (std::uint8_t lsn = 0; lsn < SECTORS_PER_TRACK; lsn++)
	{
	  std::uint8_t psn  = AppleIIDiskImage::logical_to_physical_sector_map[format][lsn];
	  std::uint8_t lsn2 = AppleIIDiskImage::physical_to_logical_sector_map[format][psn];
	  if (lsn2 != lsn)
	  {
	    std::cout << std::format("{} map, lsn {} -> psn {}, -> lsn {}\n",
				     magic_enum::enum_name(format),
				     lsn, psn, lsn2);
	    ++error_count;
	  }
	}
      });
  std::cout << std::format("error count {}\n", error_count);
  return error_count == 0;
}

void AppleIIDiskImage::load(ImageFormat format,
			    const std::filesystem::path& filename)
{
  std::ifstream file(filename,
		     std::ios_base::in | std::ios_base::binary);
  if (! file.is_open())
  {
    throw AppleIIDiskImageError("unable to open disk image to read");
  }

  for (std::uint8_t track = 0; track < TRACKS_PER_DISK; ++track)
  {
    for (std::uint8_t physical_sector = 0; physical_sector < SECTORS_PER_TRACK; ++physical_sector)
    {
      std::uint8_t logical_sector = physical_to_logical_sector_map[format][physical_sector];
      std::size_t offset = (track * SECTORS_PER_TRACK + logical_sector) * BYTES_PER_SECTOR;
      std::uint8_t* data = m_image.data() + offset;
      file.read(reinterpret_cast<char*>(data), BYTES_PER_SECTOR);
      if (file.fail())
      {
	throw AppleIIDiskImageError("error reading disk iamge");
      }
    }
  }
}

void AppleIIDiskImage::save(ImageFormat format,
			    const std::filesystem::path& filename)
{
  std::ofstream file(filename,
		     std::ios_base::out | std::ios_base::binary);
  if (! file.is_open())
  {
    throw AppleIIDiskImageError("unable to open disk image to write");
  }

  for (std::uint8_t track = 0; track < TRACKS_PER_DISK; ++track)
  {
    for (std::uint8_t physical_sector = 0; physical_sector < SECTORS_PER_TRACK; ++physical_sector)
    {
      std::uint8_t logical_sector = physical_to_logical_sector_map[format][physical_sector];
      std::size_t offset = (track * SECTORS_PER_TRACK + logical_sector) * BYTES_PER_SECTOR;
      const std::uint8_t* data = m_image.data() + offset;
      file.write(reinterpret_cast<const char*>(data), BYTES_PER_SECTOR);
      if (file.fail())
      {
	throw AppleIIDiskImageError("error writing disk iamge");
      }
    }
  }
}

void AppleIIDiskImage::read(std::uint8_t track,
			    std::uint8_t sector,
			    std::size_t sector_count,
			    std::uint8_t* data)
{
  std::size_t offset = (track * SECTORS_PER_TRACK + sector) * BYTES_PER_SECTOR;
  std::size_t count = sector_count * BYTES_PER_SECTOR;
  if ((offset + count) >= BYTES_PER_DISK)
  {
    throw AppleIIDiskImageError("read beyond end of disk image");
  }
  std::memcpy(data, m_image.data() + offset, count);
}

void AppleIIDiskImage::write(std::uint8_t track,
			     std::uint8_t sector,
			     std::size_t sector_count,
			     const std::uint8_t* data)
{
  std::size_t offset = (track * SECTORS_PER_TRACK + sector) * BYTES_PER_SECTOR;
  std::size_t count = sector_count * BYTES_PER_SECTOR;
  if ((offset + count) >= BYTES_PER_DISK)
  {
    throw AppleIIDiskImageError("write beyond end of disk image");
  }
  std::memcpy(m_image.data() + offset, data, count);
}
