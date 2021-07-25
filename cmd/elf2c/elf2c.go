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

// elf2c converts one or more Harvey ELF files to a single C source file.
// The first argument is the output file, and 2nd and succeeding ones
// are input files.
// The ELF files are assumed to contain at least one segment WITH an
// 'E' attribute (text); and one segment WITHOUT an 'E' attribute (data).
// Given a file named, e.g., cat,
// elf2c generates variables of the form:
// cat_text_start // starting address of the cat text in user VM.
// cat_text_end // ending address of the cat text in user VM.
// cat_text_len // length of the text segment
// cat_text_out // actual data for the text segment.
// Another set of these variables is generated for the data segment.
// Please note: The sizes and starts are ints. These files will not
// be large, certainly not larger than 2G as they have to fit in the
// kernel image; And, further, their starting address is easily in
// 31 bits. It's just for the bootstrap file system, which we hope
// to continue to shrink as we can use an initrd now.
//
// These numbers are easily held by an int -- even an int32.
// Please don't have this code start emitting uint64 or something.
// That's already been the cause of one bug.
// Also, for now, the text and data segments are REQUIRED.
// If gcc 10 is generating ELF files with a single RWE segment, then
// fix gcc 10.

package main

import (
	"bytes"
	"debug/elf"
	"flag"
	"fmt"
	"io"
	"io/ioutil"
	"log"
	"math"
	"os"
	"path"
)

const initialStartValue = math.MaxInt32

var dry = flag.Bool("dryrun", true, "don't really do it")

func gencode(w io.Writer, n, t string, m []byte, start, end int) {
	fmt.Fprintf(os.Stderr, "Write %v %v start %#x end %#x\n", n, t, start, end)
	fmt.Fprintf(w, "int %v_%v_start = %#x;\n", n, t, start)
	fmt.Fprintf(w, "int %v_%v_end = %#x;\n", n, t, end)
	fmt.Fprintf(w, "int %v_%v_len = %#x;\n", n, t, end-start)
	fmt.Fprintf(w, "u8 %v_%v_out[] = {\n", n, t)
	for i := start; i < end; i += 16 {
		for j := 0; i+j < end && j < 16; j++ {
			fmt.Fprintf(w, "0x%02x, ", m[j+i])
		}
		fmt.Fprintf(w, "\n")
	}
	fmt.Fprintf(w, "};\n")
}
func main() {
	flag.Parse()
	a := flag.Args()
	w := &bytes.Buffer{}
	for _, n := range a[1:] {
		f, err := elf.Open(n)
		if err != nil {
			fmt.Printf("%v %v\n", n, err)
			continue
		}
		datastart, codestart, start := initialStartValue, initialStartValue, initialStartValue
		dataend, codeend, end := -1, -1, -1
		mem := []byte{}
		for _, v := range f.Progs {
			if v.Type != elf.PT_LOAD {
				continue
			}
			fmt.Fprintf(os.Stderr, "processing %v\n", v)
			// MUST alignt to 2M page boundary.
			// then MUST allocate a []byte that
			// is the right size. And MUST
			// see if by some off chance it
			// joins to a pre-existing segment.
			// It's easier than it seems. We produce ONE text
			// array and ONE data array. So it's a matter of creating
			// a virtual memory space with an assumed starting point of
			// 0x200000, and filling it. We just grow that as needed.

			curstart := int(v.Vaddr) & ^0xfff
			curend := int(v.Vaddr) + int(v.Memsz)
			fmt.Fprintf(os.Stderr, "s %#x e %#x\n", curstart, curend)
			if curend > end {
				nmem := make([]byte, curend)
				copy(nmem, mem)
				mem = nmem
			}
			if curstart < start {
				start = curstart
			}

			if v.Flags&elf.PF_X == elf.PF_X {
				if curstart < codestart {
					codestart = curstart
				}
				if curend > codeend {
					codeend = curend
				}
				fmt.Fprintf(os.Stderr, "code s %#x e %#x\n", codestart, codeend)
			} else {
				if curstart < datastart {
					datastart = curstart
				}
				if curend > dataend {
					dataend = curend
				}
				fmt.Fprintf(os.Stderr, "data s %#x e %#x\n", datastart, dataend)
			}
			for i := 0; i < int(v.Filesz); i++ {
				if amt, err := v.ReadAt(mem[int(v.Vaddr)+i:], int64(i)); err != nil && err != io.EOF {
					log.Fatalf("%v: %v\n", amt, err)
				} else if amt == 0 {
					if i < int(v.Filesz) {
						log.Fatalf("%v: Short read: %v of %v\n", v, i, v.Filesz)
					}
					break
				} else {
					i = i + amt
					fmt.Fprintf(os.Stderr, "Total bytes read is now %v\n", i)
				}
			}
			fmt.Fprintf(os.Stderr, "Processed %v\n", v)
		}
		fmt.Fprintf(os.Stderr, "gencode\n")
		_, file := path.Split(n)
		fmt.Fprintf(w, "usize %v_main = %#x;\n", n, f.Entry)
		var msg string
		if codestart == initialStartValue {
			msg = fmt.Sprintf("codestart was never set for %s; the ELF file has no 'R E' segment.", file)
		}
		if datastart == initialStartValue {
			msg += fmt.Sprintf("datastart was never set for %s; the ELF file has no 'RW' or 'R' segment", file)
		}
		if len(msg) > 0 {
			log.Fatal(msg)
		}

		gencode(w, file, "code", mem, codestart, codeend)
		gencode(w, file, "data", mem, datastart, dataend)
	}
	if err := ioutil.WriteFile(a[0], w.Bytes(), 0444); err != nil {
		fmt.Fprintf(os.Stderr, "elf2c: write %s failed: %v\n", a[0], err)
	}

}
