// Build builds code as directed by json files.
// We slurp in the JSON, and recursively process includes.
// At the end, we issue a single cc command for all the files.
// Compilers are fast.
//
// ENVIRONMENT
//
// Needed: HARVEY, ARCH
//
// Currently only "amd64" is a valid ARCH. HARVEY should point to a harvey root.
// A best-effort to autodetect the harvey root is made if not explicitly set.
//
// Optional: CC, AR, LD, RANLIB, STRIP, SH, TOOLPREFIX
//
// These all control how the needed tools are found.
//
package main

import (
	"encoding/json"
	"flag"
	"fmt"
	"io"
	"io/ioutil"
	"log"
	"net/http"
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
	jsons map[string]bool
	path  string
	name  string
	// Projects name a whole subproject which is built independently of
	// this one. We'll need to be able to use environment variables at some point.
	Projects    []string
	PreFetch    map[string]string
	Pre         []string
	Post        []string
	Cflags      []string
	Oflags      []string
	Include     []string
	SourceFiles []string
	ObjectFiles []string
	Libs        []string
	Env         []string
	// cmd's
	SourceFilesCmd []string
	// Targets.
	Program string
	Library string
	Install string // where to place the resulting binary/lib
	Kernel  *kernel
}

type buildfile map[string]build

// UnmarshalJSON works like the stdlib unmarshal would, except it adjusts all
// paths.
func (bf *buildfile) UnmarshalJSON(s []byte) error {
	r := make(map[string]build)
	if err := json.Unmarshal(s, &r); err != nil {
		return err
	}
	for k, b := range r {
		// we're getting a copy of the struct, remember.
		b.jsons = make(map[string]bool)
		b.Projects = adjust(b.Projects)
		b.Libs = adjust(b.Libs)
		b.Cflags = adjust(b.Cflags)
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

var (
	cwd       string
	harvey    string
	shellhack bool
	regexpAll = []*regexp.Regexp{regexp.MustCompile(".")}

	// findTools looks at all env vars and absolutizes these paths
	// also respects TOOLPREFIX
	tools = map[string]string{
		"cc":     "gcc",
		"ar":     "ar",
		"ld":     "ld",
		"ranlib": "ranlib",
		"strip":  "strip",
		"sh":     "sh",
	}
	arch = map[string]bool{
		"amd64": true,
	}
	debugPrint     = flag.Bool("debug", false, "enable debug prints")
	forceShellhack = flag.Bool("shellhack", false, "spawn every command in a shell (forced on if LD_PRELOAD is set)")
)

func debug(fmt string, s ...interface{}) {
	if *debugPrint {
		log.Printf(fmt, s...)
	}
}

// fail with message, if err is not nil
func failOn(err error) {
	if err != nil {
		log.Fatalf("%v\n", err)
	}
}

func adjust(s []string) []string {
	for i, v := range s {
		s[i] = fromRoot(v)
	}
	return s
}

// Sh sends cmd to a shell. It's needed to enable $LD_PRELOAD tricks,
// see https://github.com/Harvey-OS/harvey/issues/8#issuecomment-131235178
func sh(cmd *exec.Cmd) {
	shell := exec.Command(tools["sh"])
	shell.Env = cmd.Env

	if cmd.Args[0] == tools["sh"] && cmd.Args[1] == "-c" {
		cmd.Args = cmd.Args[2:]
	}
	commandString := strings.Join(cmd.Args, " ")
	if shStdin, e := shell.StdinPipe(); e == nil {
		go func() {
			defer shStdin.Close()
			io.WriteString(shStdin, commandString)
		}()
	} else {
		log.Fatalf("cannot pipe [%v] to %s: %v", commandString, tools["sh"], e)
	}
	shell.Stderr = os.Stderr
	shell.Stdout = os.Stdout

	debug("%q | sh\n", commandString)
	failOn(shell.Run())
}

func include(f string, b *build) {
	if b.jsons[f] {
		return
	}
	b.jsons[f] = true
	log.Printf("Including %v", f)
	d, err := ioutil.ReadFile(f)
	failOn(err)
	var builds buildfile
	failOn(json.Unmarshal(d, &builds))

	for n, build := range builds {
		log.Printf("Merging %v", n)
		b.SourceFiles = append(b.SourceFiles, build.SourceFiles...)
		b.Cflags = append(b.Cflags, build.Cflags...)
		b.Oflags = append(b.Oflags, build.Oflags...)
		b.Pre = append(b.Pre, build.Pre...)
		b.Post = append(b.Post, build.Post...)
		b.Libs = append(b.Libs, build.Libs...)
		b.Projects = append(b.Projects, build.Projects...)
		b.Env = append(b.Env, build.Env...)
		b.SourceFilesCmd = append(b.SourceFilesCmd, build.SourceFilesCmd...)
		b.Program += build.Program
		b.Library += build.Library
		if build.Install != "" {
			if b.Install != "" {
				log.Fatalf("In file %s (target %s) included by %s (target %s): redefined Install.", f, n, build.path, build.name)
			}
			b.Install = build.Install
		}
		b.ObjectFiles = append(b.ObjectFiles, build.ObjectFiles...)
		// For each source file, assume we create an object file with the last char replaced
		// with 'o'. We can get smarter later.
		for _, v := range build.SourceFiles {
			f := path.Base(v)
			o := f[:len(f)-1] + "o"
			b.ObjectFiles = append(b.ObjectFiles, o)
		}

		for _, v := range build.Include {
			if !path.IsAbs(v) {
				wd := path.Dir(f)
				v = path.Join(wd, v)
			}
			include(v, b)
		}
	}
}

func appendIfMissing(s []string, v string) []string {
	for _, a := range s {
		if a == v {
			return s
		}
	}
	return append(s, v)
}

func process(f string, r []*regexp.Regexp) []build {
	log.Printf("Processing %v", f)
	var builds buildfile
	var results []build
	d, err := ioutil.ReadFile(f)
	failOn(err)
	failOn(json.Unmarshal(d, &builds))
	for n, build := range builds {
		build.name = n
		build.jsons = make(map[string]bool)
		skip := true
		for _, re := range r {
			if re.MatchString(build.name) {
				skip = false
				break
			}
		}
		if skip {
			continue
		}
		log.Printf("Run %v", build.name)
		build.jsons[f] = true
		build.path = path.Dir(f)

		// For each source file, assume we create an object file with the last char replaced
		// with 'o'. We can get smarter later.
		for _, v := range build.SourceFiles {
			f := path.Base(v)
			o := f[:len(f)-1] + "o"
			build.ObjectFiles = appendIfMissing(build.ObjectFiles, o)
		}

		for _, v := range build.Include {
			include(v, &build)
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
	failOn(ioutil.WriteFile(b.name+".c", codebuf, 0666))
}

func compile(b *build) {
	log.Printf("Building %s\n", b.name)
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
			run(b, shellhack, cmd)
		}
		return
	}
	args = append(args, b.SourceFiles...)
	cmd := exec.Command(tools["cc"], args...)
	run(b, shellhack, cmd)
}

func link(b *build) {
	log.Printf("Linking %s\n", b.name)
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
			args = append(args, b.Oflags...)
			args = append(args, "-L", fromRoot("/$ARCH/lib"))
			args = append(args, b.Libs...)
			run(b, shellhack, exec.Command(tools["ld"], args...))
		}
		return
	}
	args := []string{"-o", b.Program}
	args = append(args, b.ObjectFiles...)
	args = append(args, b.Oflags...)
	args = append(args, "-L", fromRoot("/$ARCH/lib"))
	args = append(args, b.Libs...)
	run(b, shellhack, exec.Command(tools["ld"], args...))
}

func install(b *build) {
	if b.Install == "" {
		return
	}

	log.Printf("Installing %s\n", b.name)
	failOn(os.MkdirAll(b.Install, 0755))

	switch {
	case len(b.SourceFilesCmd) > 0:
		for _, n := range b.SourceFilesCmd {
			ext := filepath.Ext(n)
			exe := n[:len(n)-len(ext)]
			move(exe, b.Install)
		}
	case len(b.Program) > 0:
		move(b.Program, b.Install)
	case len(b.Library) > 0:
		libpath := path.Join(b.Install, b.Library)
		args := append([]string{"-rs", libpath}, b.ObjectFiles...)
		run(b, shellhack, exec.Command(tools["ar"], args...))
		run(b, shellhack, exec.Command(tools["ranlib"], libpath))
	}
}

func move(from, to string) {
	final := path.Join(to, from)
	log.Printf("move %s %s\n", from, final)
	_ = os.Remove(final)
	failOn(os.Link(from, final))
	failOn(os.Remove(from))
}

func run(b *build, shellhack bool, cmd *exec.Cmd) {
	if b != nil {
		cmd.Env = append(os.Environ(), b.Env...)
	}
	cmd.Stdout = os.Stdout
	cmd.Stderr = os.Stderr
	if shellhack {
		// Sh sends cmd to a shell. It's needed to enable $LD_PRELOAD tricks, see https://github.com/Harvey-OS/harvey/issues/8#issuecomment-131235178
		shell := exec.Command(tools["sh"])
		shell.Env = cmd.Env
		shell.Stderr = os.Stderr
		shell.Stdout = os.Stdout

		commandString := strings.Join(cmd.Args, " ")
		shStdin, err := shell.StdinPipe()
		if err != nil {
			log.Fatalf("cannot pipe [%v] to %s: %v", commandString, tools["sh"], err)
		}
		go func() {
			defer shStdin.Close()
			io.WriteString(shStdin, commandString)
		}()

		log.Printf("%q | sh\n", commandString)
		failOn(shell.Run())
		return
	}
	log.Println(strings.Join(cmd.Args, " "))
	failOn(cmd.Run())
}

func fetch(target, source string) {
	out, err := os.Create(target)
	failOn(err)
	defer out.Close()
	resp, err := http.Get(source)
	failOn(err)
	defer resp.Body.Close()
	l, err := io.Copy(out, resp.Body)
	failOn(err)
	log.Printf("Fetched %v -> %v (%d bytes)\n", source, target, l)
}

func projects(b *build, r []*regexp.Regexp) {
	for _, v := range b.Projects {
		log.Printf("Doing %s\n", strings.TrimSuffix(v, ".json"))
		project(v, r)
	}
}

// assumes we are in the wd of the project.
func project(bf string, which []*regexp.Regexp) {
	cwd, err := os.Getwd()
	failOn(err)
	debug("Start new project cwd is %v", cwd)
	defer os.Chdir(cwd)
	dir := path.Dir(bf)
	root := path.Base(bf)
	debug("CD to %v and build using %v", dir, root)
	failOn(os.Chdir(dir))
	builds := process(root, which)
	debug("Processing %v: %d target", root, len(builds))
	for _, b := range builds {
		debug("Processing %v: %v", b.name, b)
		projects(&b, regexpAll)
		for dst, url := range b.PreFetch {
			fetch(dst, url)
		}
		for _, c := range b.Pre {
			run(&b, true, exec.Command(c))
		}
		buildkernel(&b)
		if len(b.SourceFiles) > 0 || len(b.SourceFilesCmd) > 0 {
			compile(&b)
		}
		if b.Program != "" || len(b.SourceFilesCmd) > 0 {
			link(&b)
		}
		install(&b)
		for _, c := range b.Post {
			run(&b, true, exec.Command(c))
		}
	}
}

func main() {
	// A small amount of setup is done in the paths*.go files. They are
	// OS-specific path setup/manipulation. "harvey" is set there and $PATH is
	// adjusted.
	var err error
	findTools(os.Getenv("TOOLPREFIX"))
	flag.Parse()
	cwd, err = os.Getwd()
	failOn(err)

	a := os.Getenv("ARCH")
	if a == "" || !arch[a] {
		s := []string{}
		for i := range arch {
			s = append(s, i)
		}
		log.Fatalf("You need to set the ARCH environment variable from: %v", s)
	}

	os.Setenv("HARVEY", harvey)

	if os.Getenv("LD_PRELOAD") != "" || *forceShellhack {
		log.Println("Using shellhack")
		shellhack = true
	}

	// If no args, assume 'build.json'
	// If 1 arg, that's a dir or file name.
	// if two args, that's a dir and a regular expression.
	f := "build.json"
	if len(flag.Args()) > 0 {
		f = flag.Arg(0)
	}
	bf, err := findBuildfile(f)
	failOn(err)

	re := []*regexp.Regexp{regexp.MustCompile(".")}
	if len(flag.Args()) > 1 {
		re = re[:0]
		for _, r := range flag.Args()[1:] {
			rx, err := regexp.Compile(r)
			failOn(err)
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
		failOn(err)
		tools[k] = v
	}
}

// disambiguate the buildfile argument
func findBuildfile(f string) (string, error) {
	try := []string{
		f,
		path.Join(f, "build.json"),
		fromRoot(path.Join("/sys/src", f+".json")),
		fromRoot(path.Join("/sys/src", f, "build.json")),
	}
	for _, p := range try {
		if fi, err := os.Stat(p); err == nil && !fi.IsDir() {
			return p, nil
		}
	}
	return "", fmt.Errorf("unable to find buildfile (tried %s)", strings.Join(try, ", "))
}
