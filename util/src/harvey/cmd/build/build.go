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
//
// Optional: CC, AR, LD, RANLIB, STRIP, TOOLPREFIX
//
// These all control how the needed tools are found.
//
package main

import (
	"bytes"
	"debug/elf"
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
	"text/template"
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

var (
	cwd    string
	harvey string

	// findTools looks at all env vars and absolutizes these paths
	// also respects TOOLPREFIX
	tools = map[string]string{
		"cc":     "gcc",
		"ar":     "ar",
		"ld":     "ld",
		"ranlib": "ranlib",
		"strip":  "strip",
		"sh":     "bash",
	}
	arch = map[string]bool{
		"amd64": true,
	}
	debugPrint = flag.Bool("debug", false, "Enable debug prints")
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

func adjust1(s string) string {
	s = os.ExpandEnv(s)
	if path.IsAbs(s) {
		return path.Join(harvey, s)
	}
	return s
}

func adjust(s []string) (r []string) {
	for _, v := range s {
		r = append(r, adjust1(v))
	}
	return
}

// send cmd to a shell
func sh(cmd *exec.Cmd) {
	shell := exec.Command(tools["sh"])
	shell.Env = cmd.Env

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

	log.Printf("[%v]", commandString)
	err := shell.Run()
	failOn(err)
}

// run cmd preserving $LD_PRELOAD tricks
func withPreloadTricks(cmd *exec.Cmd) {
	if os.Getenv("LD_PRELOAD") != "" {
		// we need a shell to enable $LD_PRELOAD tricks,
		// see https://github.com/Harvey-OS/harvey/issues/8#issuecomment-131235178
		sh(cmd)
	} else {
		cmd.Stdin = os.Stdin
		cmd.Stderr = os.Stderr
		cmd.Stdout = os.Stdout
		log.Printf("%v", cmd.Args)
		err := cmd.Run()
		failOn(err)
	}
}

func process(f, which string, ab *build) {
	r := regexp.MustCompile(which)
	if ab.jsons[f] {
		return
	}
	log.Printf("Processing %v", f)
	d, err := ioutil.ReadFile(f)
	failOn(err)
	var builds map[string]build
	err = json.Unmarshal(d, &builds)
	failOn(err)

	for n, build := range builds {
		b := ab
		build.name = n
		log.Printf("Do %v", b.name)
		if !r.MatchString(build.name) {
			continue
		}
		b.jsons[f] = true

		if len(b.jsons) == 1 {
			cwd, err := os.Getwd()
			failOn(err)

			b.path = path.Join(cwd, f)
			b.name = build.name
			b.Kernel = build.Kernel
		}

		for t, s := range(build.PreFetch) {
			b.PreFetch[t] = s
		}
		b.SourceFiles = append(b.SourceFiles, build.SourceFiles...)
		b.Cflags = append(b.Cflags, build.Cflags...)
		b.Oflags = append(b.Oflags, build.Oflags...)
		b.Pre = append(b.Pre, build.Pre...)
		b.Post = append(b.Post, build.Post...)
		b.Libs = append(b.Libs, adjust(build.Libs)...)
		b.Projects = append(b.Projects, adjust(build.Projects)...)
		b.Env = append(b.Env, build.Env...)
		b.SourceFilesCmd = append(b.SourceFilesCmd, build.SourceFilesCmd...)
		b.Program += build.Program
		b.Library += build.Library
		b.Install += build.Install
		b.ObjectFiles = append(b.ObjectFiles, adjust(build.ObjectFiles)...)

		includes := adjust(build.Include)
		for _, v := range includes {
			if !path.IsAbs(v) {
				wd := path.Dir(f)
				v = path.Join(wd, v)
			}
			process(v, ".*", b)
		}

		// For each source file, assume we create an object file with the last char replaced
		// with 'o'. We can get smarter later.

		for _, v := range build.SourceFiles {
			f := path.Base(v)
			o := f[:len(f)-1] + "o"
			b.ObjectFiles = append(b.ObjectFiles, o)
		}
	}

}

func compile(b *build) {
	// N.B. Plan 9 has a very well defined include structure, just three things:
	// /amd64/include, /sys/include, .
	args := []string{"-std=c11", "-c"}
	args = append(args, adjust([]string{"-I", os.ExpandEnv("/$ARCH/include"), "-I", "/sys/include", "-I", "."})...)
	args = append(args, adjust(b.Cflags)...)
	if len(b.SourceFilesCmd) > 0 {
		for _, i := range b.SourceFilesCmd {
			argscmd := append(args, []string{i}...)
			cmd := exec.Command(tools["cc"], argscmd...)
			cmd.Env = append(os.Environ(), b.Env...)
			withPreloadTricks(cmd)
			argscmd = args
		}
	} else {
		args = append(args, b.SourceFiles...)
		cmd := exec.Command(tools["cc"], args...)
		cmd.Env = append(os.Environ(), b.Env...)

		withPreloadTricks(cmd)
	}
}

func link(b *build) {
	if len(b.SourceFilesCmd) > 0 {
		for _, n := range b.SourceFilesCmd {
			// Split off the last element of the file
			var ext = filepath.Ext(n)
			if len(ext) == 0 {
				log.Fatalf("refusing to overwrite extension-less source file %v", n)
				continue
			}
			n = n[0 : len(n)-len(ext)]
			args := []string{"-o", n}
			f := path.Base(n)
			o := f[:len(f)] + ".o"
			args = append(args, []string{o}...)
			args = append(args, b.Oflags...)
			args = append(args, adjust([]string{"-L", os.ExpandEnv("/$ARCH/lib")})...)
			args = append(args, b.Libs...)
			cmd := exec.Command(tools["ld"], args...)
			cmd.Env = append(os.Environ(), b.Env...)

			withPreloadTricks(cmd)
		}
	} else {
		args := []string{"-o", b.Program}
		args = append(args, b.ObjectFiles...)
		args = append(args, b.Oflags...)
		args = append(args, adjust([]string{"-L", os.ExpandEnv("/$ARCH/lib")})...)
		args = append(args, b.Libs...)
		cmd := exec.Command(tools["ld"], args...)
		cmd.Env = append(os.Environ(), b.Env...)

		withPreloadTricks(cmd)
	}
}

func install(b *build) {
	if b.Install == "" {
		return
	}
	installpath := adjust([]string{os.ExpandEnv(b.Install)})
	// Make sure they're all there.
	for _, v := range installpath {
		err := os.MkdirAll(v, 0755)
		failOn(err)
	}

	if len(b.SourceFilesCmd) > 0 {
		for _, n := range b.SourceFilesCmd {
			// Split off the last element of the file
			var ext = filepath.Ext(n)
			if len(ext) == 0 {
				log.Fatalf("refusing to overwrite extension-less source file %v", n)
				continue
			}
			n = n[0 : len(n)-len(ext)]
			args := []string{n}
			args = append(args, installpath...)

			cmd := exec.Command("mv", args...)

			withPreloadTricks(cmd)
		}
	} else if len(b.Program) > 0 {
		args := []string{b.Program}
		args = append(args, installpath...)
		cmd := exec.Command("mv", args...)

		withPreloadTricks(cmd)
	} else if len(b.Library) > 0 {
		args := []string{"-rvs"}
		libpath := installpath[0] + "/" + b.Library
		args = append(args, libpath)
		for _, n := range b.SourceFiles {
			// All .o files end up in the top-level directory
			n = filepath.Base(n)
			// Split off the last element of the file
			var ext = filepath.Ext(n)
			if len(ext) == 0 {
				log.Fatalf("confused by extension-less file %v", n)
				continue
			}
			n = n[0 : len(n)-len(ext)]
			n = n + ".o"
			args = append(args, n)
		}
		cmd := exec.Command(tools["ar"], args...)

		log.Printf("*** Installing %v ***", b.Library)
		withPreloadTricks(cmd)

		cmd = exec.Command(tools["ranlib"], libpath)
		withPreloadTricks(cmd)
	}

}

func fetchOne(source string, target string) {
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
func fetch(files map[string]string) {
	for target, source := range(files){
		fetchOne(source, target)
	}
}

func run(b *build, cmd []string) {
	for _, v := range cmd {
		cmd := exec.Command(v)
		cmd.Env = append(os.Environ(), b.Env...)
		sh(cmd) // we need a shell here
	}
}

func projects(b *build) {
	for _, v := range b.Projects {
		project(v, ".*")
	}
}

func data2c(name string, path string) (string, error) {
	var out []byte
	var in []byte

	if elf, err := elf.Open(path); err == nil {
		elf.Close()
		cwd, err := os.Getwd()
		tmpf, err := ioutil.TempFile(cwd, name)
		failOn(err)

		args := []string{"-o", tmpf.Name(), path}
		cmd := exec.Command(tools["strip"], args...)
		cmd.Env = os.Environ()
		withPreloadTricks(cmd)

		in, err = ioutil.ReadAll(tmpf)
		failOn(err)

		tmpf.Close()
		os.Remove(tmpf.Name())
	} else {
		var file *os.File
		var err error

		file, err = os.Open(path)
		failOn(err)

		in, err = ioutil.ReadAll(file)
		failOn(err)

		file.Close()
	}

	total := len(in)

	out = []byte(fmt.Sprintf("static unsigned char ramfs_%s_code[] = {\n", name))
	for len(in) > 0 {
		for j := 0; j < 16 && len(in) > 0; j++ {
			out = append(out, []byte(fmt.Sprintf("0x%02x, ", in[0]))...)
			in = in[1:]
		}
		out = append(out, '\n')
	}

	out = append(out, []byte(fmt.Sprintf("0,\n};\nint ramfs_%s_len = %v;\n", name, total))...)

	return string(out), nil
}

func confcode(path string, kern *kernel) []byte {
	var rootcodes []string
	var rootnames []string
	if kern.Ramfiles != nil {
		for name, path := range kern.Ramfiles {
			code, err := data2c(name, adjust1(path))
			failOn(err)

			rootcodes = append(rootcodes, code)
			rootnames = append(rootnames, name)
		}
	}

	vars := struct {
		Path      string
		Config    kernconfig
		Rootnames []string
		Rootcodes []string
	}{
		path,
		kern.Config,
		rootnames,
		rootcodes,
	}
	tmpl, err := template.New("kernconf").Parse(`
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "../port/error.h"
#include "io.h"

void
rdb(void)
{
	splhi();
	iprint("rdb...not installed\n");
	for(;;);
}

{{ range .Rootcodes }}
{{ . }}
{{ end }}

{{ range .Config.Dev }}extern Dev {{ . }}devtab;
{{ end }}
Dev *devtab[] = {
{{ range .Config.Dev }}
	&{{ . }}devtab,
{{ end }}
	nil,
};

{{ range .Config.Link }}extern void {{ . }}link(void);
{{ end }}
void
links(void)
{
{{ range .Rootnames }}addbootfile("{{ . }}", ramfs_{{ . }}_code, ramfs_{{ . }}_len);
{{ end }}
{{ range .Config.Link }}{{ . }}link();
{{ end }}
}

#include "../ip/ip.h"
{{ range .Config.Ip }}extern void {{ . }}init(Fs*);
{{ end }}
void (*ipprotoinit[])(Fs*) = {
{{ range .Config.Ip }}	{{ . }}init,
{{ end }}
	nil,
};

#include "../port/sd.h"
{{ range .Config.Sd }}extern SDifc {{ . }}ifc;
{{ end }}
SDifc* sdifc[] = {
{{ range .Config.Sd }}	&{{ . }}ifc,
{{ end }}
	nil,
};

{{ range .Config.Uart }}extern PhysUart {{ . }}physuart;
{{ end }}
PhysUart* physuart[] = {
{{ range .Config.Uart }}	&{{ . }}physuart,
{{ end }}
	nil,
};

#define	Image	IMAGE
#include <draw.h>
#include <memdraw.h>
#include <cursor.h>
#include "screen.h"
{{ range .Config.VGA }}extern VGAdev {{ . }}dev;
{{ end }}
VGAdev* vgadev[] = {
{{ range .Config.VGA }}	&{{ . }}dev,
{{ end }}
	nil,
};

{{ range .Config.VGA }}extern VGAcur {{ . }}cur;
{{ end }}
VGAcur* vgacur[] = {
{{ range .Config.VGA }}	&{{ . }}cur,
{{ end }}
	nil,
};

Physseg physseg[8] = {
	{
		.attr = SG_SHARED,
		.name = "shared",
		.size = SEGMAXPG,
	},
	{
		.attr = SG_BSS,
		.name = "memory",
		.size = SEGMAXPG,
	},
};
int nphysseg = 8;

{{ range .Config.Code }}{{ . }}
{{ end }}

char* conffile = "{{ .Path }}";

`)

	codebuf := bytes.NewBuffer(nil)
	failOn(err)

	err = tmpl.Execute(codebuf, vars)
	failOn(err)

	return codebuf.Bytes()
}

func buildkernel(b *build) {
	if b.Kernel == nil {
		return
	}

	codebuf := confcode(b.path, b.Kernel)

	if err := ioutil.WriteFile(b.name+".c", codebuf, 0666); err != nil {
		log.Fatalf("Writing %s.c: %v", b.name, err)
	}
}

// assumes we are in the wd of the project.
func project(root, which string) {
	cwd, err := os.Getwd()
	failOn(err)
	debug("Start new project cwd is %v", cwd)
	defer os.Chdir(cwd)
	// root is allowed to be a directory or file name.
	dir := root
	if st, err := os.Stat(root); err != nil {
		log.Fatalf("%v: %v", root, err)
	} else {
		if st.IsDir() {
			root = "build.json"
		} else {
			dir = path.Dir(root)
			root = path.Base(root)
		}
	}
	debug("CD to %v and build using %v", dir, root)
	if err := os.Chdir(dir); err != nil {
		log.Fatalf("Can't cd to %v: %v", dir, err)
	}
	debug("Processing %v", root)
	b := &build{}
	b.jsons = map[string]bool{}
	b.PreFetch = map[string]string{}
	process(root, which, b)
	projects(b)
	fetch(b.PreFetch)
	run(b, b.Pre)
	buildkernel(b)
	if len(b.SourceFiles) > 0 {
		compile(b)
	}
	if len(b.SourceFilesCmd) > 0 {
		compile(b)
	}
	log.Printf("root %v program %v\n", root, b.Program)
	if b.Program != "" {
		link(b)
	}
	if b.Library != "" {
		//library(b)
		log.Printf("\n\n*** Building %v ***\n\n", b.Library)
	}
	if len(b.SourceFilesCmd) > 0 {
		link(b)
	}
	install(b)
	run(b, b.Post)
}

func main() {
	var badsetup bool
	var err error
	flag.Parse()
	cwd, err = os.Getwd()
	failOn(err)
	harvey = os.Getenv("HARVEY")

	err = findTools(os.Getenv("TOOLPREFIX"))
	failOn(err)

	if harvey == "" {
		log.Printf("You need to set the HARVEY environment variable")
		badsetup = true
	}
	a := os.Getenv("ARCH")
	if a == "" || !arch[a] {
		s := []string{}
		for i := range arch {
			s = append(s, i)
		}
		log.Printf("You need to set the ARCH environment variable from: %v", s)
		badsetup = true
	}
	if badsetup {
		os.Exit(1)
	}

	// If no args, assume '.'
	// If 1 arg, that's a dir or file name.
	// if two args, that's a dir and a regular expression.
	// TODO: more than one RE.
	dir := "."
	if len(flag.Args()) > 0 {
		dir = flag.Arg(0)
	}
	re := ".*"
	if len(flag.Args()) > 1 {
		re = flag.Arg(1)
	}
	project(dir, re)
}

func findTools(toolprefix string) (err error) {
	for k, v := range tools {
		if x := os.Getenv(strings.ToUpper(k)); x != "" {
			v = x
		}
		v = toolprefix + v
		v, err = exec.LookPath(v)
		failOn(err)
		tools[k] = v
	}
	return nil
}
