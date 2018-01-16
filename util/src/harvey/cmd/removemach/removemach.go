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
