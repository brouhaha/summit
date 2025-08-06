// crest.cc
//
// Copyright 2022-2025 Eric Smith
// SPDX-License-Identifier: GPL-3.0-only

#include <format>
#include <iostream>
#include <string>

#include <boost/program_options.hpp>

#include <magic_enum.hpp>

#include "apex.hh"
#include "apple_ii_disk_image.hh"


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
  CREATE,
};


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
	      


int main(int argc, char *argv[])
{
  Command command;
  std::string disk_image_fn;
  std::vector<std::string> host_fns;
  [[maybe_unused]] AppleIIDiskImage::ImageFormat image_format = AppleIIDiskImage::ImageFormat::DOS_ORDER;

  try
  {
    // maybe change command parsing like:
    //       https://stackoverflow.com/a/23098581/4284097

    po::options_description gen_opts("Options");
    gen_opts.add_options()
      ("help",                                           "output help message")
      ("create",                                         "create a new image from files");

    po::options_description hidden_opts("Hidden options:");
    hidden_opts.add_options()
      ("command",  po::value<Command>(&command), "command")
      ("image",    po::value<std::string>(&disk_image_fn),  "disk image filename")
      ("filename", po::value<std::vector<std::string>>(&host_fns), "host filenames");

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

    if (vm.count("filename") < 1)
    {
      throw po::validation_error(po::validation_error::at_least_one_value_required,
				 "filename");
    }
  }
  catch (po::error& e)
  {
    std::cerr << "argument error: " << e.what() << "\n";
    std::exit(1);
  }

#if 0
  std::cout << std::format("command: {}\n", magic_enum::enum_name(command));
  std::cout << std::format("image filename: \"{}\"\n", disk_image_fn);
  for (const auto& fn: host_fns)
  {
    std::cout << std::format("host filename: \"{}\"\n", fn);
  }
#endif

  apex::Disk disk;
  //disk.load(AppleIIDiskImage::ImageFormat::DOS_ORDER, disk_image_fn);

  return 0;
}
