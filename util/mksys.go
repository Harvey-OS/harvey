package main
		

import (
	"fmt"
	"io/ioutil"
	"log"
	"strings"
)

func main(){
	s, err := ioutil.ReadFile("sys.h")
	if err != nil {
		log.Fatal(err)
	}
	lines := strings.Split(string(s), "\n")
	for _, line := range lines {
		ass := "TEXT "
		ll := strings.Fields(line)
		if len(ll) < 3 {
			continue
		}
		name := strings.ToLower(ll[1])
		if name == "exits" {
			name = "_" + name
		}
		filename := name + ".s"
		if name == "seek" {
			name = "_" + name
		}
		ass = ass + fmt.Sprintf("%s(SB), 1, $0\n", name)
		if (name != "nanotime"){
			ass = ass + "\tMOVQ RARG, a0+0(FP)\n"
		}
		ass = ass + "\tMOVQ $"+ll[2]+",RARG\n"
		/* arguments in system call registers
		 * we already do this for Linux binaries;
		 * might as well extend it to NxM binaries.
		 * AX and CX are taken.
		 * R15, 14, 13, and 11 are taken.
		 * BP is RARG
		 * We want to group args as much as we can
		 * to keep assembly code simple.
		 * It's a headache.
		 * We propose to take DI, SI, DX, R10
		 * so we are compatible with linux usage.
		 * It might let us have one syscall path
		 * some day.
		 * We indicate args in registers with
		 * 0xc000 in AX.
		 * We will also minimize
		 * # args moved later; 4 is the worst case.
		 */
		ass = ass + "\tMOVQ $0xc000,AX\n"
		ass = ass + "\tMOVQ a1+0x8(FP), DI\n"
		ass = ass + "\tMOVQ a1+0x10(FP), SI\n"
		ass = ass + "\tMOVQ a1+0x18(FP), DX\n"
		ass = ass + "\tMOVQ a1+0x20(FP), R10\n"

		ass = ass + "\tSYSCALL\n\tRET\n"
		err = ioutil.WriteFile(filename, []byte(ass), 0666)
		if err != nil {
			log.Fatal(err)
		}
	}
}

