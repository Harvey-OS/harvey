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
		ass = ass + "\tMOVQ $"+ll[2]
		ass = ass + ",%r9  // Put the system call into arg 6, which is never used on Plan 9. minimizes work on system calls\n"
		ass = ass + "\tSYSCALL\n\tRET\n"
		err = ioutil.WriteFile(filename, []byte(ass), 0666)
		if err != nil {
			log.Fatal(err)
		}
	}
}

