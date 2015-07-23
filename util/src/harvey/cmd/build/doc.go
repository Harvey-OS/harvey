/*
Build builds code as directed by json files.

We slurp in the JSON, and recursively process includes.
At the end, we issue a single cc command for all the files.
Compilers are fast, so incremental builds are eschewed.

USAGE

The -v (verbose) flag is currently ignored. In the future, the default will be
for quieter builds, and verbose output can be toggled on.

The -strict (strict mode) flag is currently ignored. In the future, it will cause
the build to fail on any tool output. Meant for easily spotting compiler errors
when working.

Any number of buildfiles may be specified after the flags. They are handled in order.
The magic barewords the tool understands are:

	"utils"/"util"
	"kernel"
	"regress"
	"cmds"
	"libs"
	"klibs"
	"all"

They behave much the same as the BUILD script.

If no buildfile is specified, `${PWD}s.json` and `${PWD}.json` are tried, in that
order. For example, if build is run in /sys/src/cmd, `cmds.json` and `cmd.json`
are looked for, and the first one is run.

In The Future, an additional target syntax may be introduced: `${buildfile}:${target}`.

ENVIRONMENT

Needed: HARVEY, ARCH

Currently only "amd64" is a valid ARCH. HARVEY should point to a harvey root.
These will be guessed if not set. Set them explicitly if the builds seem to
be failing. They are guessed at if not set.

Optional: CC, AR, LD, RANLIB, STRIP, TOOLPREFIX

These all control how the needed tools are found.

BUILDFILES

buildfiles are still evolving, be sure to rebuild the build tool often.
Here's some notes on what the individual fields of the JSON files do:

Name: names the current config. For now, you can only have one config per
file, but later you may be able to have more than one.

Projects: sub-projects. Subdirectory Makefiles essentially. These get built
BEFORE pretty much anything else, including Pre commands

Pre: commands to run before compilation

Post: Commands to run after compilation

Cflags, Oflags: self-explanatory

Include: additional json files to be read in and processed. For instance
include Cflags and Oflags that many configs may use

ObjectFiles: you don't define this in the .json, it's build from the
SourceFiles element by stripping the .c and adding .o

Libs: libraries that need to be linked in

Env: things to stick in the environment.

SourceFilesCmd: list files that should be built into separate commands. This
is the mkmany paradigm; if you list "aan.c", we will first build "aan.o",
then link it to create "aan".

SourceFiles: list files that get built into a single binary. Mkone.

Program: The name of the program we want to output, assuming we're using
SourceFiles instead of SourceFilesCmd.

Install: this is the directory where your program(s) will be placed after
compiling and linking.

Library: the name of the .a file we want to generate. Currently ignored! We
just stick an ar command in the Post commands, which is pretty naughty.
*/
package main
