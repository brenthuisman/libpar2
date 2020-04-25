## libpar2

This repo is a libified version of par2cmdline. It tracks that source line for line, but instead of the commandline application, you obtain a shared library, ready for easy inclusion in your own applications.

At this time, `./compile.sh` produces a `.so` on Unix platforms. On the TODO is a similar file for Windows. Pull requests for macOS welcome!

## Motivation

I found no way easy to turn par2cmdline into a library, and after a long and unpleasant struggle with automake an libtool, I decided to simply not use the existing (auto)makefile, nor letting the linker sort out includes and symbol availability, but do it (nearly) the SQLite way and simply include all source into a single file.

## Usage

This library exposes a single function, `par2cmdline`, which takes `int,char**` parameters. These are, you guessed it, exactly what you would pass to the `par2` command. Let's say you would want the equivalent of `par2 c -r5 -n1 music.mp3`, then call par2cmdline as follows:

	const char* cmds[4] = {"c", "-r5", "-n1", "music.mp3"};
	par2cmdline(4,cmds);

Isn't it marvelous?

Because it is not in the par2cmdline readme, I'll save you some code digging and report below the possible `par2cmdline` return values:

	0: Succes.
	1: Repairable damage found.
	2: Irreparable damage found.
	3: Invalid commandline arguments.
	4: Parity file unusable.
	5: Repair failed.
	6: IO error.
	7: Internal error.
	8: Out of memory.

## Dependencies

 * None :)

### Changelog

 * 2020-04-25: v0.8.1. First release of libpar2. I'll follow `par2cmdline` versioning.
