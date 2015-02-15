/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

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
		ass = ass + "\tMOVQ a0+0x0(FP), DI\n"
		ass = ass + "\tMOVQ a1+0x8(FP), SI\n"
		ass = ass + "\tMOVQ a2+0x10(FP), DX\n"
		ass = ass + "\tMOVQ a3+0x18(FP), R10\n"
		ass = ass + "\tMOVQ a4+0x20(FP), R8\n"

		ass = ass + "\tSYSCALL\n\tRET\n"
		err = ioutil.WriteFile(filename, []byte(ass), 0666)
		if err != nil {
			log.Fatal(err)
		}
	}
}

