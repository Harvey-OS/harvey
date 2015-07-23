package main

import (
	"bytes"
	"debug/elf"
	"encoding/json"
	"flag"
	"fmt"
	"io/ioutil"
	"log"
	"os"
	"os/exec"
	"path"
	"path/filepath"
	"runtime"
	"strings"
	"text/template"
)

var (
	verbose    = flag.Bool("v", false, "be verbose (always on, for now)")
	strictMode = flag.Bool("strict", false, "strict mode (fail on any tool output) (unimplemented)")
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
	arch   string

	// findTools looks at all env vars and absolutizes these paths
	// also respects TOOLPREFIX
	tools = map[string]string{
		"cc":     "gcc",
		"ar":     "ar",
		"ld":     "ld",
		"ranlib": "ranlib",
		"strip":  "strip",
	}

	tmpl = template.New("")

	ErrNoBuildFile = fmt.Errorf("unable to find a build file!")
)

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
	for i, v := range build.Env {
		build.Env[i] = os.ExpandEnv(v)
	}

	if len(b.jsons) == 1 {
		cwd, err := os.Getwd()
		fail(err)
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
	// TODO: replace amd64 with an arch variable. Later.
	args := []string{"-c"}
	args = append(args, adjust([]string{"-I", "/amd64/include", "-I", "/sys/include", "-I", "."})...)
	args = append(args, b.Cflags...)
	if len(b.SourceFilesCmd) > 0 {
		for _, i := range b.SourceFilesCmd {
			argscmd := append(args, []string{i}...)
			cmd := mkcmd(tools["cc"], argscmd...)
			cmd.Env = append(os.Environ(), b.Env...)
			fail(cmd.Run())
			argscmd = args
		}
	} else {
		args = append(args, b.SourceFiles...)
		cmd := mkcmd(tools["cc"], args...)
		cmd.Env = append(os.Environ(), b.Env...)
		fail(cmd.Run())
	}
}

func link(b *build) {
	if len(b.SourceFilesCmd) > 0 {
		for _, n := range b.SourceFilesCmd {
			// Split off the last element of the file
			var ext = filepath.Ext(n)
			if ext == "" {
				log.Fatalf("refusing to overwrite extension-less source file %v", n)
			}
			n = n[0 : len(n)-len(ext)]
			args := []string{"-o", n}
			f := path.Base(n)
			o := f[:len(f)] + ".o"
			args = append(args, []string{o}...)
			args = append(args, b.Oflags...)
			args = append(args, adjust([]string{"-L", "/amd64/lib"})...)
			args = append(args, b.Libs...)
			cmd := mkcmd(tools["ld"], args...)
			cmd.Env = append(os.Environ(), b.Env...)
			fail(cmd.Run())
		}
	} else {
		args := []string{"-o", b.Program}
		args = append(args, b.ObjectFiles...)
		args = append(args, b.Oflags...)
		args = append(args, adjust([]string{"-L", "/amd64/lib"})...)
		args = append(args, b.Libs...)
		cmd := mkcmd(tools["ld"], args...)
		cmd.Env = append(os.Environ(), b.Env...)
		fail(cmd.Run())
	}
}

func install(b *build) {
	if b.Install == "" {
		return
	}
	installpath := adjust1(os.ExpandEnv(b.Install))
	fail(os.MkdirAll(installpath, 0755))

	switch {
	case len(b.SourceFilesCmd) > 0:
		for _, n := range b.SourceFilesCmd {
			// Split off the last element of the file
			var ext = filepath.Ext(n)
			if ext == "" {
				log.Fatalf("refusing to overwrite extension-less source file %v", n)
			}
			n = n[:len(n)-len(ext)]
			fail(mv(n, installpath))
		}
	case b.Program != "":
		fail(mv(b.Program, installpath))
	case b.Library != "":
		libpath := filepath.Join(installpath, b.Library)
		args := []string{"-rvs", libpath}
		for _, n := range b.SourceFiles {
			// All .o files end up in the top-level directory
			n = filepath.Base(n)
			// Split off the last element of the file
			var ext = filepath.Ext(n)
			if ext == "" {
				log.Fatalf("confused by extension-less file %v", n)
			}
			// replace extension
			n = n[:len(n)-len(ext)] + ".o"
			args = append(args, n)
		}
		fmt.Printf("Installing %v\n", b.Library)
		fail(mkcmd(tools["ar"], args...).Run())
		fail(mkcmd(tools["ranlib"], libpath).Run())
	}

}

func run(b *build, cmd []string) {
	for _, v := range cmd {
		cmd := mkcmd("bash", "-c", v)
		cmd.Env = append(os.Environ(), b.Env...)
		fail(cmd.Run())
	}
}

func projects(b *build) {
	sd, err := os.Getwd()
	fail(err)
	for _, v := range b.Projects {
		wd := path.Dir(v)
		f := path.Base(v)
		fail(os.Chdir(wd))
		project(f)
		fail(os.Chdir(sd))
	}
}

func data2c(name string, path string) (string, error) {
	var out []byte
	var in []byte

	if elf, err := elf.Open(path); err == nil {
		elf.Close()
		cwd, err := os.Getwd()
		tmpf, err := ioutil.TempFile(cwd, name)
		fail(err)
		args := []string{"-o", tmpf.Name(), path}
		cmd := mkcmd(tools["strip"], args...)
		cmd.Env = nil
		fail(cmd.Run())

		in, err = ioutil.ReadAll(tmpf)
		fail(err)
		tmpf.Close()
		os.Remove(tmpf.Name())
	} else {
		file, err := os.Open(path)
		fail(err)
		in, err = ioutil.ReadAll(file)
		fail(err)
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
			fail(err)
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
	fail(err)
	fail(tmpl.Execute(codebuf, vars))

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
		fmt.Printf("Building %v\n", b.Library)
	}
	if len(b.SourceFilesCmd) > 0 {
		link(b)
	}
	install(b)
	run(b, b.Post)
}

func main() {
	flag.Parse()
	*verbose = true
	if *verbose {
		log.SetFlags(log.Lshortfile | log.LstdFlags)
	}
	cwd, err := os.Getwd()
	fail(err)
	harvey = os.Getenv("HARVEY")
	if harvey == "" {
		tl, err := exec.Command("git", "rev-parse", "--show-toplevel").Output()
		fail(err)
		harvey = strings.TrimSpace(string(tl))
		fail(os.Setenv("HARVEY", harvey))
		if *verbose {
			log.Printf("guessing at harvey root, set HARVEY to be explicit (%s)", harvey)
		}
	}
	fail(findTools(os.Getenv("TOOLPREFIX")))
	if os.Getenv("ARCH") == "" {
		arch = runtime.GOARCH
		fail(os.Setenv("ARCH", arch))
		if *verbose {
			log.Printf("guessing at target arch, set ARCH to be explicit (%s)", arch)
		}
	}
	fail(os.Setenv("PATH", os.ExpandEnv("${HARVEY}/util:${PATH}")))
	targets := os.Args[1:]
	for i, v := range targets {
		switch v {
		case "utils", "util":
			targets[i] = filepath.Join(harvey, "util/util.json")
		case "kernel":
			targets[i] = filepath.Join(harvey, "sys/src/9/k10/k8cpu.json")
		case "regress":
			targets[i] = filepath.Join(harvey, "sys/src/regress/regress.json")
		case "cmds":
			targets[i] = filepath.Join(harvey, "sys/src/cmd/cmds.json")
		case "libs":
			targets[i] = filepath.Join(harvey, "sys/src/libs.json")
		case "klibs":
			targets[i] = filepath.Join(harvey, "sys/src/klibs.json")
		case "all":
			targets = []string{
				filepath.Join(harvey, "util/util.json"),
				filepath.Join(harvey, "sys/src/libs.json"),
				filepath.Join(harvey, "sys/src/klibs.json"),
				filepath.Join(harvey, "sys/src/cmd/cmds.json"),
				filepath.Join(harvey, "sys/src/9/k10/k8cpu.json"),
				filepath.Join(harvey, "sys/src/regress/regress.json"),
			}
			break
		}
	}

	if len(targets) == 0 {
		name := filepath.Base(cwd)
		try := []string{
			filepath.Join(cwd, name+"s.json"),
			filepath.Join(cwd, name+".json"),
		}
		for _, n := range try {
			fi, err := os.Stat(n)
			if os.IsNotExist(err) {
				log.Println(err)
			} else {
				log.Println(fi)
				targets = append(targets, fi.Name())
				break
			}
		}
		if len(targets) == 0 {
			for _, n := range try {
				log.Printf("\ttried %s\n", n)
			}
			fail(ErrNoBuildFile)
		}
	}

	for _, tgt := range targets {
		dir := path.Dir(tgt)
		file := path.Base(tgt)
		fail(os.Chdir(dir))
		project(file)
	}
	fail(os.Chdir(cwd))
}
