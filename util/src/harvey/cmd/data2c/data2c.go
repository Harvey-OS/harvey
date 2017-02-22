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
