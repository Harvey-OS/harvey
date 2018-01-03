// Copyright (C) 2018 Harvey OS
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

package main

import (
	"bytes"
	"flag"
	"fmt"
	"io/ioutil"
	"os"
)

func main() {
	flag.Parse()
	a := flag.Args()
	if len(a) != 3 {
		fmt.Fprintf(os.Stderr, "[%v]usage: data2s name input-file output-file\n", a)
		os.Exit(1)
	}

	n := a[0]
	i := a[1]
	in, err := ioutil.ReadFile(i)
	if err != nil {
		fmt.Fprintf(os.Stderr, "%v\n", err)
		os.Exit(1)
	}

	total := len(in)

	o := a[2]
	b := &bytes.Buffer{}
	fmt.Fprintf(b, "unsigned char %vcode[] = {\n", n)
	for len(in) > 0 {
		for j := 0; j < 16 && len(in) > 0; j++ {
			fmt.Fprintf(b, "0x%02x, ", in[0])
			in = in[1:]
		}
		fmt.Fprintf(b, "\n")

	}

	fmt.Fprintf(b, "0,\n};\nint %vlen = %v;\n", n, total)

	if err := ioutil.WriteFile(o, b.Bytes(), 0644); err != nil {
		fmt.Fprintf(os.Stderr, "Write to %s failed: %v\n", o, err)
	}
}
