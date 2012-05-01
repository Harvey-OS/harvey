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
		filename := ""
		ll := strings.Split(line, "\t")
		if len(ll) < 3 {
			continue
		}
		name := ll[1]
		if name == "seek" {
			ass = ass + "_"
			filename = filename + "_"
		}
		filename = filename + name + ".s"
		ass = ass + fmt.Sprintf("%s(SB), 1, $0\n", name)
		ass = ass + "\tMOVQ RARG, a0+0(FP)\n\tMOVQ $" + ll[2]
		ass = ass + ", RARG\n\tSYSCALL\n\tRET\n"
		err = ioutil.WriteFile(filename, []byte(ass), 0666)
		if err != nil {
			log.Fatal(err)
		}
	}
}

