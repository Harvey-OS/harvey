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

func main() {
	s, err := ioutil.ReadFile("sys.h")
	if err != nil {
		log.Fatal(err)
	}
	lines := strings.Split(string(s), "\n")
	for _, line := range lines {
		ass := ".globl "
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
		ass = ass + fmt.Sprintf("%v\n%v: ", name, name)
		// linux system call args. di si dx r10 r8 r0
		// rcx is not used because it is needed for sysenter
		ass = ass + "\tMOVQ %rcx, %r10 /* rcx gets smashed by systenter. Use r10.*/\n"
		ass = ass + "\tMOVQ $" + ll[2]
		ass = ass + ",%rax  /* Put the system call into rax, just like linux. */\n"
		ass = ass + "\tSYSCALL\n\tRET\n"
		err = ioutil.WriteFile(filename, []byte(ass), 0666)
		if err != nil {
			log.Fatal(err)
		}
	}
}
