package main

import (
	"bytes"
	"debug/elf"
	"flag"
	"fmt"
	"io/ioutil"
	"os"
	"path/filepath"
	"strings"
)

var dry = flag.Bool("dryrun", true, "don't really do it")

func main() {
	flag.Parse()
	a := flag.Args()
	for _, n := range a {
		f, err := elf.Open(n)
		if err != nil {
			fmt.Printf("%v %v\n", n, err)
			continue
		}

		s, err := f.Symbols()
		if err != nil {
			fmt.Printf("%v %v\n", n, err)
			continue
		}
		usem := false
		for _, v := range s {
			if v.Name == "m" && v.Section == elf.SHN_UNDEF {
				usem = true
				cf := strings.Split(n, ".")
				globs, err := filepath.Glob("../*/" + cf[0] + ".c")
				if err != nil || len(globs) == 0 {
					fmt.Fprintf(os.Stderr, "%v has NO source?\n", cf[0])
					continue
				}
				if len(globs) > 1 {
					fmt.Fprintf(os.Stderr, "Skipping %v has more than one source?\n", cf[0])
					continue
				}
				file := globs[0]
				fi, err := os.Stat(file)
				if err != nil {
					fmt.Fprintf(os.Stderr, "%v\n", err)
					continue
				}
				/* OK, read it in, write it out */
				b, err := ioutil.ReadFile(file)
				if err != nil {
					fmt.Fprintf(os.Stderr, "%v: %v\n", file, err)
				}
				header := []byte("extern Mach *m; // REMOVE ME\n")
				if bytes.Compare(header[:], b[0:len(header)]) == 0 {
					fmt.Fprintf(os.Stderr, "%v already done; skipping\n", file)
					continue
				}
				out := append([]byte("typedef struct Mach Mach; extern Mach *m; // REMOVE ME\n"), b...)
				if *dry {
					fmt.Fprintf(os.Stderr, "Would do %v mode %v\n", file, fi.Mode())
					continue
				}
				if err := ioutil.WriteFile(file, out, fi.Mode()); err != nil {
					fmt.Fprintf(os.Stderr, "Write %v failed: %v; git checkout %v\n", file, err, file)
				}
			}
		}
		if !usem {
			fmt.Fprintf(os.Stderr, "Ignored %v as it did not reference Mach *m\n", n)
		} else {
			fmt.Fprintf(os.Stderr, "Procssed %v as it did reference Mach *m\n", n)
		}
	}

}
