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
		fmt.Fprintf(os.Stderr, "[%v]usage: data2s name\n", a)
		os.Exit(1)
	}

	i := a[0]
	o := a[1]
	in, err := ioutil.ReadFile(i)
	if err != nil {
		fmt.Fprintf(os.Stderr, "Read %v: %v\n", a[0], err)
		os.Exit(1)
	}
	
	total := len(in)
	data := ""

	for len(in) > 0 {
		for j := 0; j < 16 && len(in) > 0; j++ {
			data += fmt.Sprintf("0x%02x, ", in[0])
			in = in[1:]
		}
		data += fmt.Sprintf("\n")
	}
		
	out := fmt.Sprintf("unsigned char %vcode[] = {\n", o)
	out += data
	out += fmt.Sprintf("0,\n};\nint %vlen = %v;\n", o, total)

	if err = ioutil.WriteFile(o, []byte(out), 0600); err != nil {
		fmt.Fprintf(os.Stderr, "%v: %v\n", o, err)
		os.Exit(1)
	}
}
