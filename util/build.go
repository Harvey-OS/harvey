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
)

type build struct {
	Name string
	Cflags []string
	Oflags []string
	Include []string
	SourceFiles []string
}

var (
	config struct {
		jsons map[string]bool
		sourcefiles []string
		cflags []string
		cwd string
		objectfiles[] string
		oflags[] string
	}
)


func fail(err error) {
	if err != nil {
		log.Fatalf("%v", err)
	}
}

func process(f string) {
	if config.jsons[f] {
		return
	}
	b, err := ioutil.ReadFile(f)
	fail(err)
	var build build
	err = json.Unmarshal(b, &build)
	fail(err)
	config.jsons[f] = true
	config.sourcefiles = append(config.sourcefiles, build.SourceFiles...)
	config.cflags = append(config.cflags, build.Cflags...)
	// For each source file, assume we create an object file with the last char replaced
	// with 'o'. We can get smarter later.
	for _, v := range build.SourceFiles {
		f := path.Base(v)
		o := f[:len(f)-1] + "o"
		config.objectfiles = append(config.objectfiles, o)
	}
	for _, v := range build.Include {
		f := path.Join(config.cwd, v)
		process(f)
	}
}

func compile() {
	args := []string{"-c"}
	args = append(args, config.cflags...)
	args = append(args, config.sourcefiles...)
	cmd := exec.Command("gcc", args...)

	cmd.Stdin = os.Stdin
	cmd.Stderr = os.Stderr
	cmd.Stdout = os.Stdout
	log.Printf("Run %v", cmd)
	err := cmd.Run()
	if err != nil {
		log.Printf("%v\n", err)
	}
}

func link() {
	args := []string{}
	args = append(args, config.oflags...)
	args = append(args, config.objectfiles...)
	cmd := exec.Command("gcc", args...)

	cmd.Stdin = os.Stdin
	cmd.Stderr = os.Stderr
	cmd.Stdout = os.Stdout
	log.Printf("Run %v", cmd)
	err := cmd.Run()
	if err != nil {
		log.Printf("%v\n", err)
	}
}
func main() {
	var err error
	config.cwd, err = os.Getwd()
	config.jsons = map[string]bool{}
	fail(err)
	f := path.Join(config.cwd, os.Args[1])
	process(f)
	compile()
	link()
}
