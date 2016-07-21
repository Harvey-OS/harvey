/*
BUILDFILE FORMAT

A buildfile is a json object containing build objects. By convention, it's
broken out onto multiple lines indented by tabs, and the keys are TitleCased.

The environment varibles HARVEY and ARCH are guarenteed to be set for the
buildfile. PATH is modified so that "$HARVEY/util" is included.

Any key that takes a path or array of paths as its value has absolute paths
re-rooted to the harvey tree and variables in the string expanded once.

The shell commands in the "Pre" and "Post" steps must be in the subset of
syntax that's accepted by both POSIX sh and rc. Practically, this means
arguments with a "=" must be single-quoted, "test" must be called as
"test" (not "["), "if" statements may not have an "else" clause, "switch"
statements may not be used, "for" statements may only have one body command,
and redirection to a file descriptor cannont be used.

BUILD OBJECT

A build object has the following keys and types:

	Projects        []string
	Pre             []string
	Post            []string
	Cflags          []string
	Oflags          []string
	Include         []string
	SourceFiles     []string
	ObjectFiles     []string
	Libs            []string
	Env             []string
	SourceFilesCmd  []string
	Program         string
	Library         string
	Install         string
	Kernel          kernel

These are the steps taken, in order:

	Env
	Include
	Projects
	Pre
	Kernel
	[compile] SourceFiles SourceFilesCmd Cflags Program
	[link] ObjectFiles Libs Oflags Library
	Install
	Post

"[compile]" and "[link]" are steps synthesized from the specified keys.

The meaning of the keys is as follows:

"Env" is an array of environment variables to be put in the environment of
every command run in a build. They are expanded once and only available for
use in other steps.

"Include" is an array of buildfiles to "merge" into the current one. This
is done before other projects are built.

"Projects" is an array of buildfiles to build in their entirety before
starting the current build.

"Pre" is an array of commands to run before starting the build.

"Kernel" is a kernel build object. See the kernel object section.

"Cflags" is an array of flags to pass to the C compiler. They are in addition
to the standard flags of

	-std=c11 -c -I /$ARCH/include -I /sys/include -I .

The standard include paths are re-rooted to the harvey tree if not on a harvey
system.

"SourceFilesCmd" is an array of C files where each one should result in an
executable. If this key is provided, "SourceFiles" and "Program" are ignored.

"SourceFiles" is an array of C files that will ultimately produce a single
binary or library, named by "Program" or "Library" respectively.

"Program" is the name of the final executable for the files specified by
"SourceFiles".

"Oflags" is an array of flags to pass to the linker. They are in addition
to the standard flags of

	-o $program $objfiles -L /$ARCH/lib $libs

The lib path is re-rooted to the harvey tree if not on a harvey system.

"ObjectFiles" is an array of strings specifying object files to be linked
into the final program specified by "Program". Any object files produced
by the preceeding "[compile]" step are automatically added to this before
beginning the "[link]" step.

"Libs" is a an array of library arguments to pass to the linker.

"Library" is the name of the archive resulting from bundling "SourceFiles"
into a library. The resulting archive has 'ranlib' run on it automatically.

"Install" is a directory where the result of the "[link]" step is moved
to. If it does not exist, it is created.

"Post" is an array of commands to run last during the build.

KERNEL OBJECT

A build object has the following keys and types:

	Systab   string
	Ramfiles map[string]string
	Config   {
		Code []string
		Dev  []string
		Ip   []string
		Link []string
		Sd   []string
		Uart []string
		VGA  []string
	}

"Systab" is the header that defines the syscall table.

"Ramfiles" is an object of name, path pairs of binaries at "path" that will
be baked into the kernel and available at a binary named "name".

"Config" is an object used to generate the kernel configuration source. "Dev",
"Ip", "Sd", "Uart", "Link", and "VGA" control which drivers of the various
types are included. "Code" is lines of arbitrary C code.

*/
package main

import (
	"flag"
	"fmt"
	"os"
	"strings"
)

var helptext = `
The buildfile is looked for at these positions, in this order:

  ./$arg
  ./$arg/build.json
  /sys/src/$arg.json
  /sys/src/$arg/build.json

If the buildfile argument is not provided, it defaults to "build.json".

After the buildfile, a number of regexps specifying targets may be provided.
If a target matches any supplied regexp, it is acted on. These regexps only
apply to the top-level buildfile.

BUILDFILE

See the build godoc for more information about the buildfile format.

ENVIRONMENT

ARCH is needed. Current acceptable vaules are: aarch64 amd64 riscv

HARVEY may be supplied to point at a harvey tree. The default on harvey is "/".
The default on Linux and OSX is to attempt to find the top level of a git
repository.
`

func init() {
	flag.Usage = func() {
		fmt.Fprintf(os.Stderr, "Usage of %s:\n", os.Args[0])
		fmt.Fprintf(os.Stderr, "  %s [options] [buildfile] [target...]\n\n", os.Args[0])
		flag.PrintDefaults()
		fmt.Fprintln(os.Stderr, helptext)
		fmt.Fprintln(os.Stderr, "Tools to be used with current settings:")
		fmt.Fprintf(os.Stderr, "  prefix ($TOOLPREFIX): %q\n", os.Getenv("TOOLPREFIX"))
		for k, v := range tools {
			fmt.Fprintf(os.Stderr, "  %s ($%s): %s\n", k, strings.ToUpper(k), v)
		}
	}
}
