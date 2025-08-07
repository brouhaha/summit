// summit.cc
//
// Copyright 2022-2025 Eric Smith
// SPDX-License-Identifier: GPL-3.0-only

#include <format>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

#include <boost/program_options.hpp>

#include <magic_enum.hpp>

#include "apex.hh"
#include "apple_ii_disk_image.hh"
#include "utility.hh"


namespace po = boost::program_options;

void conflicting_options(const boost::program_options::variables_map& vm,
			 std::initializer_list<const std::string> opts)
{
  if (opts.size() < 2)
  {
    throw std::invalid_argument("conflicting_options requires at least two options");
  }
  for (auto opt1 = opts.begin(); opt1 < opts.end(); opt1++)
  {
    if (vm.count(*opt1))
    {
      for (auto opt2 = opt1 + 1; opt2 != opts.end(); opt2++)
      {
	if (vm.count(*opt2))
	{
	  std::cerr << std::format("Options {} and {} are mutually exclusive\n", *opt1, *opt2);
	  std::exit(1);
	}
      }
    }
  }
}



enum class Command
{
  LS,
  CREATE,
  EXTRACT,
  RM,
  // for debug:
  FREE,
};


bool patterns_match(const std::vector<Apex::Filename>& patterns,
		    const Apex::Filename& filename)
{
  for (const Apex::Filename& pattern: patterns)
  {
    if (pattern.match(filename))
    {
      return true;
    }
  }
  return false;
}


void ls(const std::string& disk_image_fn,
	const std::vector<Apex::Filename>& patterns)
{
  const std::vector<Apex::Filename> wildcard
  {
    Apex::Filename("*.*"),
  };

  const std::vector<Apex::Filename>& p = patterns.size() ? patterns : wildcard;

  Apex::Disk disk;
  disk.load(AppleIIDiskImage::ImageFormat::APEX_ORDER, disk_image_fn);
  auto dir = disk.get_directory(Apex::Disk::DirectoryType::PRIMARY);
  unsigned file_count = 0;
  unsigned file_listed_count = 0;
  std::cout << "              first   block\n";
  std::cout << "filename      block   count   date\n";
  std::cout << "------------  ------  ------  ----------\n";
  for (const auto& dir_entry: dir)
  {
    if (dir_entry.get_status() == Apex::DirectoryEntry::Status::VALID)
    {
      ++file_count;
      const Apex::Filename& filename = dir_entry.get_filename();
      if (patterns_match(p, filename))
      {
	++file_listed_count;
	std::cout << std::format("{:12}  {:6d}  {:6d}  {}\n",
				 filename.to_string(),
				 dir_entry.get_first_block(),
				 dir_entry.get_block_count(),
				 dir_entry.get_date().to_string());
      }
    }
  }
  std::cout << std::format("{} of {} files listed, {} blocks used, {} blocks free of {} total blcoks\n",
			   file_listed_count,
			   file_count,
			   dir.volume_size_blocks() - dir.volume_free_blocks(),
			   dir.volume_free_blocks(),
			   dir.volume_size_blocks());
  std::cout << "\n";
};


void free(const std::string& disk_image_fn)
{
  Apex::Disk disk;
  disk.load(AppleIIDiskImage::ImageFormat::APEX_ORDER, disk_image_fn);
  auto dir = disk.get_directory(Apex::Disk::DirectoryType::PRIMARY);
  dir.debug_list_free_blocks();
};


void create(const std::string& disk_image_fn,
	    const std::vector<Apex::Filename>& patterns)
{
  (void) disk_image_fn;
  (void) patterns;
  throw std::runtime_error("create not implemented");
}


void rm(const std::string& disk_image_fn,
	const std::vector<Apex::Filename>& patterns)
{
  Apex::Disk disk;
  disk.load(AppleIIDiskImage::ImageFormat::APEX_ORDER, disk_image_fn);
  auto dir = disk.get_directory(Apex::Disk::DirectoryType::PRIMARY);
  unsigned file_deleted_count = 0;
  for (auto& dir_entry: dir)
  {
    if (dir_entry.get_status() == Apex::DirectoryEntry::Status::VALID)
    {
      const Apex::Filename& filename = dir_entry.get_filename();
      if (patterns_match(patterns, filename))
      {
	std::cout << std::format("deleting file {}\n", filename.to_string());
	dir_entry.delete_file();
	++file_deleted_count;
      }
    }
  }
  disk.save(AppleIIDiskImage::ImageFormat::APEX_ORDER, disk_image_fn);
  std::cout << std::format("{} files deleted\n", file_deleted_count);
}


void extract_file(Apex::Disk& disk,
		  const Apex::Filename& filename,
		  std::uint16_t first_block,
		  std::uint16_t block_count)
{
  (void) disk;
  std::string host_filename = utility::downcase_string(filename.to_string());
  std::cout << std::format("extracting file {}, first block {}, block count {}\n",
			   filename.to_string(),
			   first_block,
			   block_count);
  std::ofstream host_file(host_filename,
			  std::ios_base::out | std::ios_base::binary);
  if (! host_file.is_open())
  {
    throw std::runtime_error(std::format("unable to open host file \"{}\" to write", host_filename));
  }
  std::array<std::uint8_t, Apex::BYTES_PER_BLOCK> buffer;
  for (std::uint16_t block_number = first_block;
       block_number < (first_block + block_count);
       ++block_number)
  {
    disk.read(block_number, 1, buffer.data());
    host_file.write(reinterpret_cast<const char*>(buffer.data()), Apex::BYTES_PER_BLOCK);
    if (host_file.fail())
    {
      throw std::runtime_error(std::format("error writing host file \"{}\"", host_filename));
    }
  }
}


void extract(const std::string& disk_image_fn,
	     const std::vector<Apex::Filename>& patterns)
{
  const std::vector<Apex::Filename> wildcard
  {
    Apex::Filename("*.*"),
  };

  const std::vector<Apex::Filename>& p = patterns.size() ? patterns : wildcard;

  Apex::Disk disk;
  disk.load(AppleIIDiskImage::ImageFormat::APEX_ORDER, disk_image_fn);
  auto dir = disk.get_directory(Apex::Disk::DirectoryType::PRIMARY);

  std::size_t file_count = 0;
  for (const auto& dir_entry: dir)
  {
    if (dir_entry.get_status() == Apex::DirectoryEntry::Status::VALID)
    {
      const Apex::Filename& filename = dir_entry.get_filename();
      if (patterns_match(p, filename))
      {
	++file_count;
	extract_file(disk,
		     filename,
		     dir_entry.get_first_block(),
		     dir_entry.get_block_count());
      }
    }
  }
  std::cout << std::format("{} files extracted\n", file_count);
}


void validate(boost::any& v,
	      const std::vector<std::string>& values,
	      Command*,
	      int)
{
  const std::string& input = values.at(0);

  auto command = magic_enum::enum_cast<Command>(input, magic_enum::case_insensitive);
  if (! command.has_value())
  {
    throw po::validation_error(po::validation_error::invalid_option_value,
			       "unrecognzied command");
  }
  v = boost::any(command.value());
}


#if 0
void validate(boost::any& v,
	      const std::vector<std::string>& values,
	      Apex::Filename*,
	      int)
{
  try
  {
    Apex::Filename f(values.at(0));
    v = boost::any(f);
  }
  catch (const Apex::FilenameError& e)
  {
    throw po::validation_error(po::validation_error::invalid_option_value,
			       e.what());
  }
}
#endif	      


int main(int argc, char *argv[])
{
  Command command;
  std::string disk_image_fn;
  std::vector<std::string> pattern_strings;
  std::vector<Apex::Filename> patterns;
  [[maybe_unused]] AppleIIDiskImage::ImageFormat image_format = AppleIIDiskImage::ImageFormat::DOS_ORDER;

  try
  {
    // maybe change command parsing like:
    //       https://stackoverflow.com/a/23098581/4284097

    po::options_description gen_opts("Options");
    gen_opts.add_options()
      ("help",                                           "output help message");

    po::options_description hidden_opts("Hidden options:");
    hidden_opts.add_options()
      ("command",  po::value<Command>(&command), "command")
      ("image",    po::value<std::string>(&disk_image_fn),  "disk image filename")
#if 0
      ("filename", po::value<std::vector<Apex::Filename>>(&patterns), "host filenames")
#else
      ("filename", po::value<std::vector<std::string>>(&pattern_strings), "host filenames")
#endif
      ;

    po::positional_options_description positional_opts;
    positional_opts.add("command",   1);
    positional_opts.add("image",     1);
    positional_opts.add("filename", -1);

    po::options_description cmdline_opts;
    cmdline_opts.add(gen_opts).add(hidden_opts);

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).
	      options(cmdline_opts).positional(positional_opts).run(), vm);
    po::notify(vm);

    if (vm.count("help"))
    {
      std::cerr << "Usage: " << argv[0] << " [options]\n\n";
      std::cerr << gen_opts << "\n";
      std::exit(0);
    }

    if (vm.count("command") != 1)
    {
      throw po::validation_error(po::validation_error::at_least_one_value_required,
				 "command");
    }


    if (vm.count("image") < 1)
    {
      throw po::validation_error(po::validation_error::at_least_one_value_required,
				 "image");
    }

    switch (command)
    {
    case Command::LS:
    case Command::EXTRACT:
      break;
    case Command::FREE:
      if (vm.count("filename") > 0)
      {
	throw po::validation_error(po::validation_error::invalid_option);
      }
      break;
    case Command::CREATE:
    case Command::RM:
      if (vm.count("filename") < 1)
      {
	throw po::validation_error(po::validation_error::at_least_one_value_required,
				   "filename");
      }
      break;
    }
  }
  catch (po::error& e)
  {
    std::cerr << "argument error: " << e.what() << "\n";
    std::exit(1);
  }

  for (const std::string& pattern_string: pattern_strings)
  {
    patterns.emplace_back(pattern_string);
  }

#if 0
  std::cout << std::format("command: {}\n", magic_enum::enum_name(command));
  std::cout << std::format("image filename: \"{}\"\n", disk_image_fn);
  for (const Apex::Filename& pattern: patterns)
  {
    std::cout << std::format("file pattern: \"{}\"\n", pattern.to_string());
  }
#endif

  switch (command)
  {
  case Command::LS:
    ls(disk_image_fn, patterns);
    break;

  case Command::EXTRACT:
    extract(disk_image_fn, patterns);
    break;

  case Command::CREATE:
    create(disk_image_fn, patterns);
    break;

  case Command::RM:
    rm(disk_image_fn, patterns);
    break;

  case Command::FREE:
    free(disk_image_fn);
    break;
  }

  return 0;
}
