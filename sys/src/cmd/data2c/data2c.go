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
