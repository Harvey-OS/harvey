package main

import (
	"flag"
	"fmt"
	"io/ioutil"
	"os"
)

func main() {
	flag.Parse()
	a := flag.Args()
	if len(a) != 2 {
		fmt.Fprintf(os.Stderr, "[%v]usage: data2s name input-file (writes to stdout)\n", a)
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

	fmt.Printf("unsigned char %vcode[] = {\n", n)
	for len(in) > 0 {
		for j := 0; j < 16 && len(in) > 0; j++ {
			fmt.Printf("0x%02x, ", in[0])
			in = in[1:]
		}
		fmt.Printf("\n")

	}

	fmt.Printf("0,\n};\nint %vlen = %v;\n", n, total)
}
