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
		/* 
		 * N.B. we should only move the # required,
		 * rather than the worst case.
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

