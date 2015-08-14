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
	"fmt"
	"io"
	"io/ioutil"
	"log"
	"os"
	"os/exec"
	"path"
	"path/filepath"
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
)

func fail(err error) {
	if err != nil {
		log.Fatalf("%v", err)
	}
}

func adjust1(s string) string {
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

func sh(cmd *exec.Cmd){
	bash := exec.Command(tools["sh"])
	bash.Env = cmd.Env
	bash.Stderr = os.Stderr
	bash.Stdout = os.Stdout
	commandString := strings.Join(cmd.Args, " ")

	if bashIn, e := bash.StdinPipe(); e == nil {

		go func(){
			defer bashIn.Close()
			io.WriteString(bashIn, commandString)
		}()

		log.Printf("[%v]", commandString)
		err := bash.Run()
		if err != nil {
			log.Fatalf("%v\n", err)
		}
	} else {
		log.Fatalf("cannot pipe [%v] to %s: %v", commandString, tools["sh"], e)
	}
}

func process(f string, b *build) {
	if b.jsons[f] {
		return
	}
	log.Printf("Processing %v", f)
	d, err := ioutil.ReadFile(f)
	fail(err)
	var build build
	err = json.Unmarshal(d, &build)
	fail(err)
	b.jsons[f] = true

	if len(b.jsons) == 1 {
		cwd, err := os.Getwd()
		if err != nil {
			log.Fatalf("%v", err)
		}
		b.path = path.Join(cwd, f)
		b.Name = build.Name
		b.Kernel = build.Kernel
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

	// For each source file, assume we create an object file with the last char replaced
	// with 'o'. We can get smarter later.

	for _, v := range build.SourceFiles {
		f := path.Base(v)
		o := f[:len(f)-1] + "o"
		b.ObjectFiles = append(b.ObjectFiles, o)
	}

	b.ObjectFiles = append(b.ObjectFiles, adjust(build.ObjectFiles)...)

	includes := adjust(build.Include)
	for _, v := range includes {
		if !path.IsAbs(v) {
			wd := path.Dir(f)
			v = path.Join(wd, v)
		}
		process(v, b)
	}
}

func compile(b *build) {
	// N.B. Plan 9 has a very well defined include structure, just three things:
	// /amd64/include, /sys/include, .
	args := []string{"-c"}
	args = append(args, adjust([]string{"-I", os.ExpandEnv("/$ARCH/include"), "-I", "/sys/include", "-I", "."})...)
	args = append(args, b.Cflags...)
	if len(b.SourceFilesCmd) > 0 {
		for _, i := range b.SourceFilesCmd {
			argscmd := append(args, []string{i}...)
			cmd := exec.Command(tools["cc"], argscmd...)
			cmd.Env = append(os.Environ(), b.Env...)
			sh(cmd)
			argscmd = args
		}
	} else {
		args = append(args, b.SourceFiles...)
		cmd := exec.Command(tools["cc"], args...)
		cmd.Env = append(os.Environ(), b.Env...)

		sh(cmd)
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

			sh(cmd)
		}
	} else {
		args := []string{"-o", b.Program}
		args = append(args, b.ObjectFiles...)
		args = append(args, b.Oflags...)
		args = append(args, adjust([]string{"-L", os.ExpandEnv("/$ARCH/lib")})...)
		args = append(args, b.Libs...)
		cmd := exec.Command(tools["ld"], args...)
		cmd.Env = append(os.Environ(), b.Env...)

		sh(cmd)
	}
}

func install(b *build) {
	if b.Install == "" {
		return
	}
	installpath := adjust([]string{os.ExpandEnv(b.Install)})
	// Make sure they're all there.
	for _, v := range installpath {
		if err := os.MkdirAll(v, 0755); err != nil {
			log.Fatalf("%v", err)
		}
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

			sh(cmd)
		}
	} else if len(b.Program) > 0 {
		args := []string{b.Program}
		args = append(args, installpath...)
		cmd := exec.Command("mv", args...)

		sh(cmd)
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
		sh(cmd)

		cmd = exec.Command(tools["ranlib"], libpath)
		sh(cmd)
	}

}

func run(b *build, cmd []string) {
	for _, v := range cmd {
		cmd := exec.Command(v)
		cmd.Env = append(os.Environ(), b.Env...)
		sh(cmd)
	}
}

func projects(b *build) {
	for _, v := range b.Projects {
		wd := path.Dir(v)
		f := path.Base(v)
		cwd, err := os.Getwd()
		fail(err)
		os.Chdir(wd)
		project(f)
		os.Chdir(cwd)
	}
}

func data2c(name string, path string) (string, error) {
	var out []byte
	var in []byte

	if elf, err := elf.Open(path); err == nil {
		elf.Close()
		cwd, err := os.Getwd()
		tmpf, err := ioutil.TempFile(cwd, name)
		if err != nil {
			log.Fatalf("%v\n", err)
		}
		args := []string{"-o", tmpf.Name(), path}
		cmd := exec.Command(tools["strip"], args...)
		cmd.Env = os.Environ()
		sh(cmd)

		in, err = ioutil.ReadAll(tmpf)
		if err != nil {
			log.Fatalf("%v\n", err)
		}
		tmpf.Close()
		os.Remove(tmpf.Name())
	} else {
		var file *os.File
		var err error
		if file, err = os.Open(path); err != nil {
			log.Fatalf("%v", err)
		}
		in, err = ioutil.ReadAll(file)
		if err != nil {
			log.Fatalf("%v\n", err)
		}
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
			if err != nil {
				log.Fatalf("%v\n", err)
			}
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
	if err != nil {
		log.Fatalf("%v\n", err)
	}
	err = tmpl.Execute(codebuf, vars)
	if err != nil {
		log.Fatalf("%v\n", err)
	}

	return codebuf.Bytes()
}

func buildkernel(b *build) {

	if b.Kernel == nil {
		return
	}

	codebuf := confcode(b.path, b.Kernel)

	if err := ioutil.WriteFile(b.Name+".c", codebuf, 0666); err != nil {
		log.Fatalf("Writing %s.c: %v", b.Name, err)
	}

}

// assumes we are in the wd of the project.
func project(root string) {
	b := &build{}
	b.jsons = map[string]bool{}
	process(root, b)
	projects(b)
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
	cwd, err = os.Getwd()
	fail(err)
	harvey = os.Getenv("HARVEY")
	if err := findTools(os.Getenv("TOOLPREFIX")); err != nil {
		fail(err)
	}
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
	dir := path.Dir(os.Args[1])
	file := path.Base(os.Args[1])
	err = os.Chdir(dir)
	fail(err)
	project(file)
}

func findTools(toolprefix string) (err error) {
	for k, v := range tools {
		if x := os.Getenv(strings.ToUpper(k)); x != "" {
			v = x
		}
		v = toolprefix + v
		v, err = exec.LookPath(v)
		if err != nil {
			return err
		}
		tools[k] = v
	}
	return nil
}
