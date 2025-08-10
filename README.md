# Summit - Apex disk image utility

## Introduction

Summit is command-line utility to manipulate Apex disk images.
Summit is written in C++20.

Summit development is hosted at the
[Summit Github repository](https://github.com/brouhaha/summit/).

## License

Summit is Copyright 2025 Eric Smith.

Summit is Free Software, licensed under the terms of the
[GNU General Public License 3.0](https://www.gnu.org/licenses/gpl-3.0.en.html)
(GPL version 3.0 only, not later versions).

## Apex History

The 6502 Group was founded in 1975, originally as a special interest group
of the Denver Amateur Computer Society. Due to the shortage of available
6502 development software, members of the 6502 group created editors,
assemblers, language interpreters, and even compilers for the 6502,
initially for homebrew systems, or systems from The Digital Group, but
eventually for the Apple II as well.

The XPL0 programming language was created by Peter J.R. Boyle, as was the
Apex operating system. The command interpreter ("executive") of Apex, as
well as many of the provided utility programs, were written in XPL0.

## Building Summit

Summit requires the
[Magic Enum](https://github.com/Neargye/magic_enum)
header-only library.

The build system uses
[SCons](https://scons.org),
which requires
[Python](https://www.python.org/)
3.6 or later.

To build Summit, make sure the Magic Enum headers are in
your system include path, per their documentation. From the top level
directory (above the "src" directory), type "scons". The resulting
executable will be build/posix/summit.

## Cross-compiling Summit for Windows

Summit can be cross-compiled on a Linux host for execution on Windows (32-bit and 64-bit).
Use a "target=win32" or "target=win64" option on the SCons command line. If the
target isn't specified, it defaults to the most recent target that was specified.
The exeuctable, and a .msi installer, will be found in build/win32 or build/win64,
as appropriate.

## Wildcards

Summit can process simple wildcards, '*' and '?', in filename patterns to
be matched against Apex disk image directory entries. Depending on the host
operating system, there are some caveats:

* On Linux/BSD/MacOS/Unix/Posix systems, the shell will expand wildcards.
  For commands in which it is desired to have summit match a wildcard pattern
  against the Apex disk image directory, the pattern should be quoted with
  single quotes (apostrophes) to prevent the shell from expanding the wildcard
  from the host directory. For example,
  `summit ls disk.img a*.*` might not work as expected, if the host system
  has any files starting with an 'a' and containing a '.'. Instead, use
  `summit ls disk.img 'a*.*'\.

* On Windows, the command prompt shell does not expand wildcards. For the
  create and insert commands, individual files will need to be specified
  on the command line. However, for other commands such as insert and ls,
  it is not necessary to put single quotes around the pattern.

## Summit commands

Summit is executed from a command line. There is two mandatory
positional arguments, which are the command to be executed, and the filename
of the Apex disk image. These may be followed by filenames or patterns for
use by the command

* `summit ls disk.img` produces a directory listing of the Apex disk image. Optional
  arguments are interpreted as patterns to match. For instance,
  `summit ls disk.img *.p65` will list only the files with the extension ".P65".
  Multiple patterns may be given, in which case only files that match at least one
  of the patterns will be listed.

* `summit free disk.img` produces a more detailed list of free blocks present
  in the Apex disk image. This was intended for use debugging Summit, and this
  command may be removed in the future.

* `summit extract disk.img` will extract all of the files in the Apex disk image
  into host files. Alphabetic characters in the host filenames will be in lower
  case. One or more patterns may be given, in which case only files that match
  at least one of the patterns will be extracted. No conversions (e.g., of newlines)
  are performed; the extraction is in raw binary format, of a multiple of 256 bytes
  (the Apex allocation block size). Note that Apex text files end with a control-Z
  character; anything in an extracted test file beyond the control-Z should be
  disregarded.

* `summit insert disk.img [host filenames...]` will insert host files into the
  image. No conversions (e.g., of newlines) are performed. If the host file is
  not a multiple of 256 bytes, the remainder of the last block of the Apex file
  will contain unspecified data.

* `summit create disk.img [host filenames...]` will create a new disk image, and
  optionally insert host files into the image as per the `insert` command.

* `summit rm disk.img [pattern...]` will delete files from the Apex disk
  image.

## Limitations

* Summit currently performs raw binary file insertion and extraction only.

* The Summit command line parsing is currently very crude. The command line
  syntax is subject to change.

