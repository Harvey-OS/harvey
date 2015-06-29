// Build builds code as directed by json files.
// We slurp in the JSON, and recursively process includes.
// At the end, we issue a single gcc command for all the files.
// Compilers are fast.
package main

import (
	//	"fmt"
	"encoding/json"
	"io/ioutil"
	"log"
	"os"
	"os/exec"
	"path"
	"path/filepath"
)

type build struct {
	// jsons is unexported so can not be set in a .json file
	jsons map[string]bool
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
}

var (
	cwd    string
	harvey string
)

func fail(err error) {
	if err != nil {
		log.Fatalf("%v", err)
	}
}

func adjust(s []string) (r []string) {
	for _, v := range s {
		if path.IsAbs(v) {
			v = path.Join(harvey, v)
		}
		r = append(r, v)
	}
	return
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

	for _, v := range build.Include {
		wd := path.Dir(f)
		f := path.Join(wd, v)
		process(f, b)
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
			cmd := exec.Command("gcc", argscmd...)
			cmd.Env = append(os.Environ(), b.Env...)
			cmd.Stdin = os.Stdin
			cmd.Stderr = os.Stderr
			cmd.Stdout = os.Stdout
			log.Printf("%v", cmd.Args)
			err := cmd.Run()
			if err != nil {
				log.Fatalf("%v\n", err)
			}
			argscmd = args
		}
	} else {
		args = append(args, b.SourceFiles...)
		cmd := exec.Command("gcc", args...)
		cmd.Env = append(os.Environ(), b.Env...)

		cmd.Stdin = os.Stdin
		cmd.Stderr = os.Stderr
		cmd.Stdout = os.Stdout
		log.Printf("%v", cmd.Args)
		err := cmd.Run()
		if err != nil {
			log.Fatalf("%v\n", err)
		}
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
			args = append(args, adjust([]string{"-L", "/amd64/lib"})...)
			args = append(args, b.Libs...)
			cmd := exec.Command("ld", args...)
			cmd.Env = append(os.Environ(), b.Env...)

			cmd.Stdin = os.Stdin
			cmd.Stderr = os.Stderr
			cmd.Stdout = os.Stdout
			log.Printf("%v", cmd.Args)
			err := cmd.Run()
			if err != nil {
				log.Fatalf("%v\n", err)
			}
		}
	} else {
		args := []string{"-o", b.Program}
		args = append(args, b.ObjectFiles...)
		args = append(args, b.Oflags...)
		args = append(args, adjust([]string{"-L", "/amd64/lib"})...)
		args = append(args, b.Libs...)
		cmd := exec.Command("ld", args...)
		cmd.Env = append(os.Environ(), b.Env...)

		cmd.Stdin = os.Stdin
		cmd.Stderr = os.Stderr
		cmd.Stdout = os.Stdout
		log.Printf("%v", cmd.Args)
		err := cmd.Run()
		if err != nil {
			log.Fatalf("%v\n", err)
		}
	}
}

func install(b *build) {
	if b.Install == "" {
		return
	}
	installpath := adjust([]string{os.ExpandEnv(b.Install)})

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

			cmd.Stdin = os.Stdin
			cmd.Stderr = os.Stderr
			cmd.Stdout = os.Stdout
			log.Printf("%v", cmd.Args)
			err := cmd.Run()
			if err != nil {
				log.Fatalf("%v\n", err)
			}
		}
	} else if len(b.Program) > 0 {
		args := []string{b.Program}
		args = append(args, installpath...)
		cmd := exec.Command("mv", args...)

		cmd.Stdin = os.Stdin
		cmd.Stderr = os.Stderr
		cmd.Stdout = os.Stdout
		log.Printf("%v", cmd.Args)
		err := cmd.Run()
		if err != nil {
			log.Fatalf("%v\n", err)
		}
	} else if len(b.Library) > 0 {
		args := []string{"rv"}
		args = append(args, installpath[0] + b.Program)
		files, err := filepath.Glob("*.o")
		if err != nil {
			log.Fatalf("%v\n", err)
		}
		args = append(args, files...)
		cmd := exec.Command("ar", args...)

		cmd.Stdin = os.Stdin
		cmd.Stderr = os.Stderr
		cmd.Stdout = os.Stdout
		log.Printf("**********INSTALLING A LIBRARY!!!!*************")
		log.Printf("%v", cmd.Args)
		err = cmd.Run()
		if err != nil {
			log.Fatalf("%v\n", err)
		}
	}

}

func run(b *build, cmd []string) {
	for _, v := range cmd {
		cmd := exec.Command("bash", "-c", v)
		cmd.Env = append(os.Environ(), b.Env...)
		cmd.Stderr = os.Stderr
		cmd.Stdout = os.Stdout
		log.Printf("%v", cmd.Args)
		err := cmd.Run()
		if err != nil {
			log.Fatalf("%v\n", err)
		}
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

// assumes we are in the wd of the project.
func project(root string) {
	b := &build{}
	b.jsons = map[string]bool{}
	process(root, b)
	projects(b)
	run(b, b.Pre)
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
	var err error
	cwd, err = os.Getwd()
	fail(err)
	harvey = os.Getenv("HARVEY")
	if harvey == "" {
		log.Fatalf("You need to set the HARVEY environment variable")
	}
	f := path.Join(cwd, os.Args[1])
	project(f)
}
