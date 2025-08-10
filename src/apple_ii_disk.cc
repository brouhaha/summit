// apple_ii_disk.cc
//
// Copyright 2025 Eric Smith
// SPDX-License-Identifier: GPL-3.0-only

#include <cstring>
#include <format>
#include <fstream>
#include <iostream>

#include <magic_enum_utility.hpp>

#include "apple_ii_disk.hh"

namespace AppleII
{

  static constexpr uint8_t dos_order_phys_to_log_table[] =
  { 0x0, 0x7, 0xe, 0x6, 0xd, 0x5, 0xc, 0x4, 0xb, 0x3, 0xa, 0x2, 0x9, 0x1, 0x8, 0xf };

  static constexpr uint8_t prodos_order_phys_to_log_table[] =
  { 0x0, 0x8, 0x1, 0x9, 0x2, 0xa, 0x3, 0xb, 0x4, 0xc, 0x5, 0xd, 0x6, 0xe, 0x7, 0xf };
						    
  static constexpr uint8_t cpm_order_phys_to_log_table[] =
  { 0x0, 0xb, 0x6, 0x1, 0xc, 0x7, 0x2, 0xd, 0x8, 0x3, 0xe, 0x9, 0x4, 0xf, 0xa, 0x5 };

  static constexpr uint8_t apex_order_phys_to_log_table[] =
  { 0x0, 0xe, 0xd, 0xc, 0xb, 0xa, 0x9, 0x8, 0x7, 0x6, 0x5, 0x4, 0x3, 0x2, 0x1, 0xf };

  static const magic_enum::containers::array<AppleII::DiskImage::ImageFormat, AppleII::DiskGeometry> geometry
  {
    /* RAW */             DiskGeometry { 256,  0, 1,  0, nullptr },
    /* THIRTEEN_SECTOR */ DiskGeometry { 256, 13, 1, 35, nullptr },
    /* DOS_ORDER */       DiskGeometry { 256, 16, 1, 35, dos_order_phys_to_log_table },
    /* PRODOS_ORDER */    DiskGeometry { 265, 16, 1, 35, prodos_order_phys_to_log_table },
    /* CPM_ORDER */       DiskGeometry { 256, 16, 1, 35, cpm_order_phys_to_log_table },
    /* APEX_ORDER */      DiskGeometry { 256, 16, 1, 35, apex_order_phys_to_log_table },
  };

  DiskError::DiskError(const std::string& what):
    std::runtime_error("Apple II disk image error: " + what)
  {
  }

  DiskImage::DiskImage(ImageFormat format):
    m_format(format),
    m_image(get_bytes_per_disk(format), 0)
  {
  }

  const DiskGeometry& DiskImage::get_geometry(ImageFormat format)
  {
    return geometry[format];
  }

  std::size_t DiskImage::get_bytes_per_disk(ImageFormat format)
  {
    const DiskGeometry& geom = geometry[format];
    return geom.bytes_per_sector * geom.sectors * geom.heads * geom.cylinders;
  }

  DiskImage::ImageFormat DiskImage::get_format() const
  {
    return m_format;
  }

  void DiskImage::set_format(ImageFormat format)
  {
    m_format = format;
    m_image.resize(get_bytes_per_disk(format), 0);
  }

  void DiskImage::load(const std::filesystem::path& filename)
  {
    std::ifstream file(filename,
		       std::ios_base::in | std::ios_base::binary);
    if (! file.is_open())
    {
      throw DiskError("unable to open disk image to read");
    }

    if (geometry[m_format].deinterleave_table)
    {
      for (std::uint8_t track = 0; track < geometry[m_format].cylinders; ++track)
      {
	for (std::uint8_t physical_sector = 0; physical_sector < geometry[m_format].sectors; ++physical_sector)
	{
	  std::uint8_t logical_sector = geometry[m_format].deinterleave_table[physical_sector];
	  std::size_t offset = (track * geometry[m_format].sectors + logical_sector) * geometry[m_format].bytes_per_sector;
	  std::uint8_t* data = m_image.data() + offset;
	  file.read(reinterpret_cast<char*>(data), geometry[m_format].bytes_per_sector);
	  if (file.fail())
	  {
	    throw DiskError("error reading disk iamge");
	  }
	}
      }
    }
    else
    {
      file.read(reinterpret_cast<char*>(m_image.data()), m_image.size());
    }
  }

  void DiskImage::save(const std::filesystem::path& filename) const
  {
    std::ofstream file(filename,
		       std::ios_base::out | std::ios_base::binary);
    if (! file.is_open())
    {
      throw DiskError("unable to open disk image to write");
    }

    if (geometry[m_format].deinterleave_table)
    {
      for (std::uint8_t track = 0; track < geometry[m_format].cylinders; ++track)
      {
	for (std::uint8_t physical_sector = 0; physical_sector < geometry[m_format].sectors; ++physical_sector)
	{
	  std::uint8_t logical_sector = geometry[m_format].deinterleave_table[physical_sector];
	  std::size_t offset = (track * geometry[m_format].sectors + logical_sector) * geometry[m_format].bytes_per_sector;
	  const std::uint8_t* data = m_image.data() + offset;
	  file.write(reinterpret_cast<const char*>(data), geometry[m_format].bytes_per_sector);
	  if (file.fail())
	  {
	    throw DiskError("error writing disk iamge");
	  }
	}
      }
    }
    else
    {
      file.write(reinterpret_cast<const char*>(m_image.data()), m_image.size());
    }
  }

  void DiskImage::read(std::uint8_t track,
		       std::uint8_t head,
		       std::uint8_t sector,
		       std::size_t sector_count,
		       std::uint8_t* data) const
  {
    std::size_t byte_offset = ((track * geometry[m_format].heads + head) * geometry[m_format].sectors + sector) * geometry[m_format].bytes_per_sector;
    std::size_t byte_count = sector_count * geometry[m_format].bytes_per_sector;
    if ((byte_offset + byte_count) >= m_image.size())
    {
      throw DiskError("read beyond end of disk image");
    }
    std::memcpy(data, m_image.data() + byte_offset, byte_count);
  }

  void DiskImage::write(std::uint8_t track,
			std::uint8_t head,
			std::uint8_t sector,
			std::size_t sector_count,
			const std::uint8_t* data)
  {
    std::size_t byte_offset = ((track * geometry[m_format].heads + head) * geometry[m_format].sectors + sector) * geometry[m_format].bytes_per_sector;
    std::size_t byte_count = sector_count * geometry[m_format].bytes_per_sector;
    if ((byte_offset + byte_count) >= m_image.size())
    {
      throw DiskError("write beyond end of disk image");
    }
    std::memcpy(m_image.data() + byte_offset, data, byte_count);
  }

} // end namespace AppleII

