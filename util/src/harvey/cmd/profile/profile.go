package main

import (
	"debug/elf"
	"flag"
	"fmt"
	"io/ioutil"
	"sort"
)

type Symbol struct {
	Addr uint64
	Name string
}

type Symbols []*Symbol

func (s Symbols) Len() int      { return len(s) }
func (s Symbols) Swap(i, j int) { s[i], s[j] = s[j], s[i] }

type ByAddress struct{ Symbols }

func (s ByAddress) Less(i, j int) bool { return s.Symbols[i].Addr < s.Symbols[j].Addr }

var profile = flag.String("profile", "", "name of file containing profile data")
var kernel = flag.String("kernel", "", "kernel to profile against")
var symbolTable = []*Symbol{}

func bytesToInt(array []byte, offset int) uint32 {
	return uint32(array[offset]) | (uint32(array[offset+1]) << 8) | (uint32(array[offset+2]) << 16) | (uint32(array[offset+3]) << 24)
}

func bytesToLong(array []byte, offset int) uint64 {
	return uint64(bytesToInt(array, offset)) | (uint64(bytesToInt(array, offset+4)) << 32)
}

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

func findFunction(addr uint64) string {
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

	offset := 0
	for offset < len(backtraces) {
		topOfStack := true
		header1 := bytesToInt(backtraces, offset)
		//timestamp := bytesToLong(backtraces, offset+8)
		//coreId := (header1 >> 16) & 0xffff
		numTraces := header1 & 0xff
		offset += 16
		//fmt.Printf("timestamp: %d, core id: %d\n", timestamp, coreId)
		traceEnd := offset + int(numTraces*8)
		for ; offset < traceEnd; offset += 8 {
			if topOfStack {
				addr := bytesToLong(backtraces, offset)
				fmt.Printf("stack: %x, method %s\n", addr, findFunction(addr))
			}
			topOfStack = true
		}
	}
}
