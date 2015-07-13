package main

import (
	"encoding/json"
	"fmt"
	"io/ioutil"
	"os"
)

func main() {
	if len(os.Args) < 2 {
		fmt.Printf("usage: jsonpretty input.json [output.json]\n")
		os.Exit(1)
	}

	buf, err := ioutil.ReadFile(os.Args[1])
	if err != nil {
		fmt.Printf("%v\n", err)
		os.Exit(1)
	}

	var jsmap map[string]interface{}
	if err := json.Unmarshal(buf, &jsmap); err != nil {
		fmt.Printf("%v\n", err)
		os.Exit(1)
	}

	for key, val := range jsmap {
		switch tval := val.(type) {
		case map[string]interface{}:
			if len(tval) == 0 {
				delete(jsmap, key)
			}
		case []interface{}:
			if len(tval) == 0 {
				delete(jsmap, key)
			}
		}
	}

	buf, err = json.MarshalIndent(jsmap, "", "\t")
	if err != nil {
		fmt.Printf("%v\n", err)
		os.Exit(1)
	}
	buf = append(buf, '\n')

	if len(os.Args) == 3 {
		ioutil.WriteFile(os.Args[2], buf, 0666)
	} else {
		os.Stdout.Write(buf)
	}
}
