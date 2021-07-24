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
	"encoding/binary"
	"flag"
	"fmt"
	"io"
	"io/ioutil"
	"os"
	"sort"
)

type Header struct {
	Sz   byte
	_    byte
	Core uint16
	_    uint16
	Ver  byte
	Sig  byte
	Time uint64
}

type pc uint64

type Record struct {
	Header
	pcs []pc
}

type Symbol struct {
	Addr uint64
	Name string
}

type Symbols []*Symbol

func (s Symbols) Len() int      { return len(s) }
func (s Symbols) Swap(i, j int) { s[i], s[j] = s[j], s[i] }

type ByAddress struct{ Symbols }

func (s ByAddress) Less(i, j int) bool { return s.Symbols[i].Addr < s.Symbols[j].Addr }

var (
	profile     = flag.String("profile", "", "name of file containing profile data")
	debug       = flag.Bool("d", false, "Debug printing")
	kernel      = flag.String("kernel", "", "kernel to profile against")
	symbolTable []*Symbol
)

func loadSymbols(kernel *elf.File) {
	syms, err := kernel.Symbols()
	if err != nil {
		fmt.Println(err)
		return
	}

	for _, sym := range syms {
		value := sym.Value | 0xffffffff00000000
		symbolTable = append(symbolTable, &Symbol{value, sym.Name})
	}
	sort.Sort(ByAddress{symbolTable})
}

func findFunction(pc pc) string {
	addr := uint64(pc)
	var prevSym *Symbol
	for _, sym := range symbolTable {
		if sym.Addr > addr && prevSym != nil {
			return prevSym.Name
		}
		prevSym = sym
	}
	return "*** Not Found ***"
}

func main() {
	flag.Parse()
	kernelElf, err := elf.Open(*kernel)
	if err != nil {
		fmt.Println(err)
		return
	}
	loadSymbols(kernelElf)

	backtraces, err := ioutil.ReadFile(*profile)
	if err != nil {
		fmt.Println(err)
		return
	}

	b := bytes.NewBuffer(backtraces)
	var numRecords int
	var records []Record
	for {
		var h Header
		if err := binary.Read(b, binary.LittleEndian, &h); err != nil {
			if err == io.EOF {
				break
			}
			fmt.Fprintf(os.Stderr, "header: %v\n", err)
			os.Exit(1)
		}
		if *debug {
			fmt.Fprintf(os.Stderr, "%d: %v\n", numRecords, h)
		}

		if h.Sig != 0xee {
			fmt.Fprintf(os.Stderr, "Record %d: sig is 0x%x, not 0xee", numRecords, h.Sig)
			os.Exit(1)
		}
		pcs := make([]pc, h.Sz)
		if err := binary.Read(b, binary.LittleEndian, pcs); err != nil {
			fmt.Fprintf(os.Stderr, "data: %v\n", err)
			os.Exit(1)
		}
		numRecords++
		records = append(records, Record{Header: h, pcs: pcs})
	}

	for _, r := range records {
		fmt.Printf("0x%x: ", r.Time)
		for _, pc := range r.pcs {
			fmt.Printf("[0x%x, %s] ", pc, findFunction(pc))
		}
		fmt.Printf("\n")
	}
}
