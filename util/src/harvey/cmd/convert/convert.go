package main

import (
	"encoding/json"
	"flag"
	"io/ioutil"
	"log"
	"os"
	"path/filepath"
)

var config = struct {
	Harvey   string
	Args     []string
	Except   map[string]bool
	CmdName  string
	FullPath string
	Src      string
	Uroot    string
	Cwd      string
	Bbsh     string

	Goroot    string
	Gosrcroot string
	Arch      string
	Goos      string
	Gopath    string
	TempDir   string
	Go        string
	Debug     bool
	Fail      bool
}{
	Harvey: os.Getenv("HARVEY"),
	Except: map[string]bool{"sysconf.json": true,},
}

type fixup func(string, map[string]interface{})

func cflags(n string, jsmap map[string]interface{}) {
	if _, ok := jsmap["Cflags"]; ok {
		log.Printf("Deleting Cflags from %v", n)
		delete(jsmap, "Cflags")
		// TODO: once we have another ARCH, use it.
		a := []string{"/amd64/include/cflags.json"}
		switch tval := jsmap["Include"].(type) {
		case []interface{}:
			for _, v := range tval {
				a = append(a, v.(string))
			}
		}
		jsmap["Include"] = a
	}
}

func removeempty(n string, jsmap map[string]interface{}) {
	for key, val := range jsmap {
		switch tval := val.(type) {
		case map[string]interface{}:
			log.Printf("%s: tval %s", n, tval)
			if len(tval) == 0 {
				delete(jsmap, key)
			}
		case []interface{}:
			if len(tval) == 0 {
				delete(jsmap, key)
			}
		}
	}
}

func checkname(n string, jsmap map[string]interface{}) {
	if _, ok := jsmap["Name"]; !ok {
		log.Print("File %v has no \"Name\" key", n)
	}
}

func one(n string, f ...fixup) error {
	buf, err := ioutil.ReadFile(n)
	if err != nil {
		return err
	}

	var jsmap map[string]interface{}
	if err := json.Unmarshal(buf, &jsmap); err != nil {
		return err
	}
	if config.Debug {
		log.Printf("%v: %v", n, jsmap)
	}

	mapname := jsmap["Name"].(string)
	delete(jsmap, "Name")
	var nmap = make(map[string]map[string]interface{})
	nmap[mapname] = jsmap
	buf, err = json.MarshalIndent(nmap, "", "\t")
	if err != nil {
		return err
	}
	buf = append(buf, '\n')

	if config.Debug {
		os.Stdout.Write(buf)
	} else {
		ioutil.WriteFile(n, buf, 0666)
	}
	return nil
}

func init() {
	if config.Harvey == "" {
		log.Fatalf("Please set $HARVEY")
	}

	if len(config.Args) == 0 {
		config.Args = []string{config.Harvey}
	}
}

func main() {
	var err error

	flag.BoolVar(&config.Debug, "d", true, "Enable debug prints")
	flag.Parse()
	config.Args = flag.Args()
	for _, n := range config.Args {
		err = filepath.Walk(n, func(name string, fi os.FileInfo, err error) error {
			if err != nil {
				return err
			}
			if fi.IsDir() {
				return nil
			}
			n := fi.Name()
			if len(n) < 5 || n[len(n)-5:] != ".json" {
				return nil
			}
			if config.Except[fi.Name()] {
				return nil
			}
			todo := []fixup{removeempty, checkname}
			log.Printf("process %s", name)
			err = one(name, todo...)
			if err != nil {
				log.Printf("%s: %s\n", name, err)
				return err
			}
			return nil
		})

		if err != nil {
			log.Fatal("%v", err)

		}
	}
}
