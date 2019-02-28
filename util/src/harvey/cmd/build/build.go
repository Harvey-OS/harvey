/*
Copyright 2018 Harvey OS Team

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

// Build builds code as directed by json files.
// We slurp in the JSON, and recursively process includes.
// At the end, we issue a single cc command for all the files.
// Compilers are fast.
//
// ENVIRONMENT
//
// Needed: CC, HARVEY, ARCH
//
// HARVEY should point to a harvey root.
// A best-effort to autodetect the harvey root is made if not explicitly set.
//
// Optional: AR, LD, RANLIB, STRIP, SH, TOOLPREFIX
//
// These all control how the needed tools are found.
//
package main

import (
	"encoding/json"
	"errors"
	"flag"
	"fmt"
	"io"
	"io/ioutil"
	"log"
	"os"
	"os/exec"
	"path"
	"path/filepath"
	"regexp"
	"strings"
)

type kernconfig struct {
	Code []string
	Dev  []string
	Ip   []string
	Link []string
	Sd   []string
	Uart []string
	VGA  []string
}

type kernel struct {
	Systab   string
	Config   kernconfig
	Ramfiles map[string]string
}

type build struct {
	// jsons is unexported so can not be set in a .json file
	jsons []string
	path  string
	Name  string
	// Projects name a whole subproject which is built independently of
	// this one. We'll need to be able to use environment variables at some point.
	Projects    []string
	Pre         []string
	Post        []string
	Cflags      []string
	Oflags      []string
	Include     []string
	SourceFiles []string
	ObjectFiles []string
	Libs        []string
	Env         []string
	SourceDeps  []string
	// cmd's
	SourceFilesCmd []string
	// Targets.
	Program string
	Library string
	Install string // where to place the resulting binary/lib
	Kernel  *kernel
}

type buildfile []build

func globby(s []string) []string {
	all := []string{}

	for _, n := range s {
		l, err := filepath.Glob(n)
		if err != nil || len(l) == 0 {
			all = append(all, n)
		} else {
			all = append(all, l...)
		}
	}

	debug("Glob of '%v' is '%v'", s, all)
	return all
}

// UnmarshalJSON works like the stdlib unmarshal would, except it adjusts all
// paths.
func (bf *buildfile) UnmarshalJSON(s []byte) error {
	r := make([]build, 0)
	if err := json.Unmarshal(s, &r); err != nil {
		return err
	}

	for k, b := range r {
		// we're getting a copy of the struct, remember.
		b.jsons = []string{}
		b.Projects = adjust(b.Projects)
		b.Libs = adjust(b.Libs)
		b.SourceDeps = adjust(b.SourceDeps)
		b.Cflags = adjust(b.Cflags)
		b.Oflags = adjust(b.Oflags)
		b.SourceFiles = globby(adjust(b.SourceFiles))
		b.SourceFilesCmd = globby(adjust(b.SourceFilesCmd))
		b.ObjectFiles = adjust(b.ObjectFiles)
		b.Include = adjust(b.Include)
		b.Install = fromRoot(b.Install)
		for i, e := range b.Env {
			b.Env[i] = os.ExpandEnv(e)
		}
		r[k] = b
	}
	*bf = r
	return nil
}

func (b *build) includedJson(filename string) bool {
	for _, includedJson := range b.jsons {
		if includedJson == filename {
			return true
		}
	}
	return false
}

var (
	harvey    string
	regexpAll = []*regexp.Regexp{regexp.MustCompile(".")}

	// findTools looks at all env vars and absolutizes these paths
	// also respects TOOLPREFIX
	tools = map[string]string{
		"cc":     "gcc",
		"ar":     "ar",
		"ld":     "ld",
		"ranlib": "ranlib",
		"strip":  "strip",
	}
	arch = map[string]bool{
		"amd64":   true,
		"riscv":   true,
		"aarch64": true,
	}
	debugPrint = flag.Bool("debug", false, "enable debug prints")
	depends    = flag.Bool("D", false, "Do dependency checking")
	shellhack  = flag.Bool("shellhack", false, "spawn every command in a shell (forced on if LD_PRELOAD is set)")
)

func debug(fmt string, s ...interface{}) {
	if *debugPrint {
		log.Printf(fmt, s...)
	}
}

func fail(err error) {
	if err != nil {
		log.Fatalf("%s\n", err.Error())
	}
}

func adjust(s []string) []string {
	for i, v := range s {
		s[i] = fromRoot(v)
	}
	return s
}

// return the given absolute path as an absolute path rooted at the harvey tree.
func fromRoot(p string) string {
	expandedPath := os.ExpandEnv(p)
	if path.IsAbs(expandedPath) {
		expandedPath = path.Join(harvey, expandedPath)
	}

	if strings.Contains(p, "$CC") {
		// Travis has versioned CCs of the form clang-X.Y.  We don't want to have
		// a file for each version of the compilers, so check if the versioned
		// file exists first.  If it doesn't, fall back to the unversioned file.
		expandedCc := os.Getenv("CC")
		expandedCcTokens := strings.Split(expandedCc, "-")
		fallbackCc := expandedCcTokens[0]
		//fmt.Printf(">>>CC=%v fallback=%v\n", expandedCc, fallbackCc)
		if len(expandedCcTokens) > 1 {
			if _, err := os.Stat(expandedPath); err != nil {
				if os.IsNotExist(err) {
					oldCc := os.Getenv("CC")
					os.Setenv("CC", fallbackCc)
					expandedPath = fromRoot(p)
					os.Setenv("CC", oldCc)
				}
			}
		}
	}

	return expandedPath
}

func include(f string, targ string, b *build) {
	debug("include(%s, %s, %v)", f, targ, b)

	if b.includedJson(f) {
		return
	}
	b.jsons = append(b.jsons, f)

	log.Printf("Including %s", f)
	builds := unmarshalBuild(f)

	for _, build := range builds {
		t := target(&build)
		if t != "" {
			targ = t
			break
		}
	}

	for _, build := range builds {
		log.Printf("Merging %s", build.Name)
		b.Cflags = append(b.Cflags, build.Cflags...)
		b.Oflags = append(b.Oflags, build.Oflags...)
		b.Pre = append(b.Pre, build.Pre...)
		b.Post = append(b.Post, build.Post...)
		b.Libs = append(b.Libs, build.Libs...)
		b.Projects = append(b.Projects, build.Projects...)
		b.Env = append(b.Env, build.Env...)
		for _, v := range build.SourceFilesCmd {
			_, t := cmdTarget(&build, v)
			if uptodate(t, append(b.SourceDeps, v)) {
				continue
			}
			b.SourceFilesCmd = append(b.SourceFilesCmd, v)
		}
		if build.Install != "" {
			if b.Install != "" && build.Install != b.Install {
				log.Fatalf("In file %s (target %s) included by %s (target %s): redefined Install: was %s, redefined to %s.", f, b.Name, build.path, build.Name, b.Install, build.Install)
			}
			b.Install = build.Install
		}
		b.ObjectFiles = append(b.ObjectFiles, build.ObjectFiles...)
		// For each source file, assume we create an object file with the last char replaced
		// with 'o'. We can get smarter later.
		b.SourceDeps = append(b.SourceDeps, build.SourceDeps...)

		var s []string
		for _, v := range build.SourceFiles {
			if uptodate(targ, append(b.SourceDeps, v)) {
				continue
			}
			s = append(s, v)
		}
		if *depends && build.Program != "" {
			if len(s) == 0 {
				build.SourceFiles = []string{}
				build.Program = ""
			}
		}

		if *depends && b.Library != "" {
			build.SourceFiles = s
		}

		for _, v := range build.SourceFiles {
			b.SourceFiles = append(b.SourceFiles, v)
			fi := path.Base(v)
			ext := path.Ext(v)
			o := fi[:len(fi)-len(ext)+1] + "o"
			b.ObjectFiles = append(b.ObjectFiles, o)
		}

		for _, v := range build.Include {
			if !path.IsAbs(v) {
				wd := path.Dir(f)
				v = path.Join(wd, v)
			}
			include(v, targ, b)
		}
		b.Program += build.Program
		b.Library += build.Library
	}
}

func contains(a []string, s string) bool {
	for i := range a {
		if a[i] == s {
			return true
		}
	}
	return false
}

func unmarshalBuild(f string) buildfile {
	d, err := ioutil.ReadFile(f)
	fail(err)

	var builds buildfile
	fail(json.Unmarshal(d, &builds))
	return builds
}

func process(f string, r []*regexp.Regexp) []build {
	log.Printf("Processing %s", f)
	var results []build

	builds := unmarshalBuild(f)

	for _, build := range builds {
		build.jsons = []string{}
		skip := true
		for _, re := range r {
			if re.MatchString(build.Name) {
				skip = false
				break
			}
		}
		if skip {
			continue
		}
		log.Printf("Running %s", build.Name)

		build.jsons = append(build.jsons, f)
		build.path = path.Dir(f)

		// Figure out which of these are up to date.
		var s []string
		for _, v := range build.SourceFilesCmd {
			_, targ := cmdTarget(&build, v)
			if uptodate(targ, append(build.SourceDeps, v)) {
				continue
			}
			s = append(s, v)
		}
		build.SourceFilesCmd = s
		// For each source file, assume we create an object file with the last char replaced
		// with 'o'. We can get smarter later.
		t := target(&build)
		deps := targetDepends(&build)
		debug("\ttarget is '%s', deps are '%v'", t, deps)
		for _, v := range build.SourceFiles {
			if uptodate(t, append(deps, v)) {
				continue
			}
			s = append(s, v)
		}

		if *depends && build.Program != "" {
			if len(s) == 0 {
				build.SourceFiles = []string{}
				build.Program = ""
			}
		}

		if *depends && build.Library != "" {
			build.SourceFiles = s
		}

		for _, v := range build.SourceFiles {
			f := path.Base(v)
			ext := path.Ext(f)
			l := len(f) - len(ext) + 1
			o := f[:l]
			o += "o"
			if !contains(build.ObjectFiles, o) {
				build.ObjectFiles = append(build.ObjectFiles, o)
			}
		}

		for _, v := range build.Include {
			include(v, t, &build)
		}
		results = append(results, build)
	}
	return results
}

func buildkernel(b *build) {
	if b.Kernel == nil {
		return
	}
	codebuf := confcode(b.path, b.Kernel)
	fail(ioutil.WriteFile(b.Name+".c", codebuf, 0666))
}

func uptodate(n string, d []string) bool {
	debug("uptodate: %s, %v\n", n, d)
	if !*depends {
		debug("\t no\n")
		return false
	}

	fi, err := os.Stat(n)
	// If it does not exist, by definition it's not up to date
	if err != nil {
		debug("\t target '%s' doesn't exist\n", n)
		return false
	}

	m := fi.ModTime()

	debug("older: time is %v\n", m)
	if len(d) == 0 {
		log.Fatalf("update has nothing to check for %v", n)
	}
	for _, d := range d {
		debug("\tCheck %s:", d)
		di, err := os.Stat(d)
		if err != nil {
			return false
		}
		if !di.ModTime().Before(m) {
			debug("%v is newer\n", di.ModTime())
			return false
		}
		debug("%v is older\n", di.ModTime())
	}

	debug("all is older\n")
	return true
}

func targetDepends(b *build) []string {
	return append(b.SourceDeps, fromRoot("/sys/include/libc.h"), fromRoot("/$ARCH/include/u.h"))
}

func target(b *build) string {
	if b.Program != "" {
		return path.Join(b.Install, b.Program)
	}
	if b.Library != "" {
		return path.Join(b.Install, b.Library)
	}
	return ""
}

func cmdTarget(b *build, n string) (string, string) {
	ext := filepath.Ext(n)
	exe := n[:len(n)-len(ext)]
	return exe, b.Install
}

func compile(b *build) {
	log.Printf("Building %s\n", b.Name)
	// N.B. Plan 9 has a very well defined include structure, just three things:
	// /amd64/include, /sys/include, .
	args := []string{
		"-std=c11", "-c",
		"-I", fromRoot("/$ARCH/include"),
		"-I", fromRoot("/sys/include"),
		"-I", ".",
	}
	args = append(args, b.Cflags...)
	if len(b.SourceFilesCmd) > 0 {
		for _, i := range b.SourceFilesCmd {
			cmd := exec.Command(tools["cc"], append(args, i)...)
			run(b, *shellhack, cmd)
		}
		return
	}
	args = append(args, b.SourceFiles...)
	cmd := exec.Command(tools["cc"], args...)
	run(b, *shellhack, cmd)
}

func link(b *build) {
	log.Printf("Linking %s\n", b.Name)
	if len(b.SourceFilesCmd) > 0 {
		for _, n := range b.SourceFilesCmd {
			// Split off the last element of the file
			var ext = filepath.Ext(n)
			if len(ext) == 0 {
				log.Fatalf("refusing to overwrite extension-less source file %v", n)
				continue
			}
			n = n[:len(n)-len(ext)]
			f := path.Base(n)
			o := f[:len(f)] + ".o"
			args := []string{"-o", n, o}
			args = append(args, "-L", fromRoot("/$ARCH/lib"))
			args = append(args, b.Libs...)
			args = append(args, b.Oflags...)
			run(b, *shellhack, exec.Command(tools["ld"], args...))
		}
		return
	}
	args := []string{"-o", b.Program}
	args = append(args, b.ObjectFiles...)
	args = append(args, "-L", fromRoot("/$ARCH/lib"))
	args = append(args, b.Libs...)
	args = append(args, b.Oflags...)
	run(b, *shellhack, exec.Command(tools["ld"], args...))
}

func install(b *build) {
	if b.Install == "" {
		return
	}

	log.Printf("Installing %s\n", b.Name)
	fail(os.MkdirAll(b.Install, 0755))

	switch {
	case len(b.SourceFilesCmd) > 0:
		for _, n := range b.SourceFilesCmd {
			move(cmdTarget(b, n))
		}
	case len(b.Program) > 0:
		move(b.Program, b.Install)
	case len(b.Library) > 0:
		libpath := path.Join(b.Install, b.Library)
		args := append([]string{"-rs", libpath}, b.ObjectFiles...)
		run(b, *shellhack, exec.Command(tools["ar"], args...))
		run(b, *shellhack, exec.Command(tools["ranlib"], libpath))
	}
}

func move(from, to string) {
	final := path.Join(to, from)
	log.Printf("move %s %s\n", from, final)
	_ = os.Remove(final)
	fail(os.Link(from, final))
	fail(os.Remove(from))
}

func run(b *build, pipe bool, cmd *exec.Cmd) {
	sh := os.Getenv("SHELL")
	if sh == "" {
		sh = "sh"
	}

	if b != nil {
		cmd.Env = append(os.Environ(), b.Env...)
	}
	cmd.Stdout = os.Stdout
	cmd.Stderr = os.Stderr
	if pipe {
		// Sh sends cmd to a shell. It's needed to enable $LD_PRELOAD tricks, see https://github.com/Harvey-OS/harvey/issues/8#issuecomment-131235178
		shell := exec.Command(sh)
		shell.Env = cmd.Env
		shell.Stderr = os.Stderr
		shell.Stdout = os.Stdout

		commandString := cmd.Args[0]
		for _, a := range cmd.Args[1:] {
			if strings.Contains(a, "=") {
				commandString += " '" + a + "'"
			} else {
				commandString += " " + a
			}
		}
		shStdin, err := shell.StdinPipe()
		if err != nil {
			log.Fatalf("cannot pipe [%v] to %s: %v", commandString, sh, err)
		}
		go func() {
			defer shStdin.Close()
			io.WriteString(shStdin, commandString)
		}()

		log.Printf("%q | %s\n", commandString, sh)
		fail(shell.Run())
		return
	}
	log.Println(strings.Join(cmd.Args, " "))
	fail(cmd.Run())
}

func projects(b *build, r []*regexp.Regexp) {
	for _, v := range b.Projects {
		f, err := findBuildfile(v)
		log.Printf("Doing %s\n", f)
		if err != nil {
			log.Println(err)
		}
		project(f, r)
	}
}

func dirPop(s string) {
	fmt.Printf("Leaving directory `%v'\n", s)
	fail(os.Chdir(s))
}

func dirPush(s string) {
	fmt.Printf("Entering directory `%v'\n", s)
	fail(os.Chdir(s))
}

func runCmds(b *build, s []string) {
	for _, c := range s {
		args := adjust(strings.Split(c, " "))
		var exp []string
		for _, v := range args {
			e, err := filepath.Glob(v)
			debug("glob %v to %v err %v", v, e, err)
			if len(e) == 0 || err != nil {
				exp = append(exp, v)
			} else {
				exp = append(exp, e...)
			}
		}
		run(b, *shellhack, exec.Command(exp[0], exp[1:]...))
	}
}

// assumes we are in the wd of the project.
func project(bf string, which []*regexp.Regexp) {
	cwd, err := os.Getwd()
	fail(err)
	debug("Start new project cwd is %v", cwd)
	defer dirPop(cwd)
	dir := path.Dir(bf)
	root := path.Base(bf)
	debug("CD to %v and build using %v", dir, root)
	dirPush(dir)
	builds := process(root, which)
	debug("Processing %v: %d target", root, len(builds))
	for _, b := range builds {
		debug("Processing %v: %v", b.Name, b)
		projects(&b, regexpAll)
		runCmds(&b, b.Pre)
		buildkernel(&b)
		if len(b.SourceFiles) > 0 || len(b.SourceFilesCmd) > 0 {
			compile(&b)
		}
		if b.Program != "" || len(b.SourceFilesCmd) > 0 {
			link(&b)
		}
		install(&b)
		runCmds(&b, b.Post)
	}
}

func main() {
	log.SetFlags(0)

	// A small amount of setup is done in the paths*.go files. They are
	// OS-specific path setup/manipulation. "harvey" is set there and $PATH is
	// adjusted.
	flag.Parse()
	findTools(os.Getenv("TOOLPREFIX"))

	if os.Getenv("CC") == "" {
		log.Fatalf("You need to set the CC environment variable (e.g. gcc, clang, clang-3.6, ...)")
	}

	a := os.Getenv("ARCH")
	if a == "" || !arch[a] {
		s := []string{}
		for i := range arch {
			s = append(s, i)
		}
		log.Fatalf("You need to set the ARCH environment variable from: %v", s)
	}

	// ensure this is exported, in case we used a default value
	os.Setenv("HARVEY", harvey)

	if os.Getenv("LD_PRELOAD") != "" {
		log.Println("Using shellhack")
		*shellhack = true
	}

	// If there is'n args, we search for a 'build.json' file
	// Otherwise the first argument could be
	// A path to a json file
	// or a directory containing the 'build.json' file
	// or a regular expression to apply assuming 'build.json'
	// Further arguments are considered regex.
	consumedArgs := 0
	var bf string
	if flag.NArg() > 0 {
		f, err := findBuildfile(flag.Arg(0))
		fail(err)

		if f == "" {
			fb, err := findBuildfile("build.json")
			fail(err)
			bf = fb
		} else {
			consumedArgs = 1
			bf = f
		}
	} else {
		f, err := findBuildfile("build.json")
		fail(err)
		bf = f
	}

	re := []*regexp.Regexp{regexp.MustCompile(".")}
	if len(flag.Args()) > consumedArgs {
		re = re[:0]
		for _, r := range flag.Args()[consumedArgs:] {
			rx, err := regexp.Compile(r)
			fail(err)
			re = append(re, rx)
		}
	}
	project(bf, re)
}

func findTools(toolprefix string) {
	var err error
	for k, v := range tools {
		if x := os.Getenv(strings.ToUpper(k)); x != "" {
			v = x
		}
		v, err = exec.LookPath(toolprefix + v)
		fail(err)
		tools[k] = v
	}
}

func isDir(f string) (bool, error) {
	fi, err := os.Stat(f)
	if err != nil {
		return false, err
	}
	return fi.IsDir(), nil
}

// disambiguate the buildfile argument
func findBuildfile(f string) (string, error) {
	if !strings.HasSuffix(f, ".json") {
		b, err := isDir(f)
		if err != nil {
			// the path didn't exist
			return "", errors.New("unable to find buildfile " + f)
		}
		if b {
			f = path.Join(f, "build.json")
		} else {
			// this is a file without .json suffix
			return "", errors.New("buildfile must be a .json file, " + f + " is not a valid name")
		}
	}
	if b, err := isDir(f); err != nil || b {
		return "", errors.New("unable to find buildfile " + f)
	}
	return f, nil
}
