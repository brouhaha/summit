// apple_ii_disk.hh
//
// Copyright 2025 Eric Smith
// SPDX-License-Identifier: GPL-3.0-only

#ifndef APPLE_II_DISK_HH
#define APPLE_II_DISK_HH

#include <cstdint>
#include <filesystem>
#include <stdexcept>
#include <vector>

#include <magic_enum.hpp>
#include <magic_enum_containers.hpp>

namespace AppleII
{

  struct DiskError: public std::runtime_error
  { DiskError(const std::string& what); };

  struct DiskGeometry
  {
    std::uint16_t bytes_per_sector;
    std::uint8_t sectors;
    std::uint8_t heads;
    std::uint8_t cylinders;
    const std::uint8_t* deinterleave_table;
  };

  class DiskImage
  {
  public:
    enum class ImageFormat
    {
      RAW,             // no sector interleave
      THIRTEEN_SECTOR, // 13 sector, like DOS before 3.3
      DOS_ORDER,       // mostly 2:1 sector interleave, in descending order
      PRODOS_ORDER,    // 2:1 sector interleave
      CPM_ORDER,       // 3:1 sector interleave
      APEX_ORDER,      // XXX HACK ALERT - 2:1 sector interleave, but read in DOS order?
    };

    DiskImage(ImageFormat format = ImageFormat::DOS_ORDER);

    static const DiskGeometry& get_geometry(ImageFormat format);
    static std::size_t get_bytes_per_disk(ImageFormat format);

    ImageFormat get_format() const;
    void set_format(ImageFormat format);

    void load(const std::filesystem::path& filename);
    void save(const std::filesystem::path& filename) const;

    void read(std::uint8_t track,
	      std::uint8_t head,
	      std::uint8_t sector,
	      std::size_t sector_count,
	      std::uint8_t* data) const;

    void write(std::uint8_t track,
	       std::uint8_t head,
	       std::uint8_t sector,
	       std::size_t sector_count,
	       const std::uint8_t* data);

  protected:
    ImageFormat m_format;
    std::vector<std::uint8_t> m_image;
  };

} // end namespace AppleII

#endif // APPLE_II_DISK_HH
