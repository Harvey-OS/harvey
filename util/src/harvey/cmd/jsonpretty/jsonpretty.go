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
