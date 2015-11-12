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
	Except: map[string]bool{"cflags.json": true, "9": true, "libauthcmd.json": true, "awk.json": true, "bzip2": true, "klibc.json": true, "kcmds.json": true, "klib.json": true, "kernel.json": true, "syscall.json": true},
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
		log.Printf("File %v has no \"Name\" key", n)
	}
}

func one(n string, f ...fixup) error {
	buf, err := ioutil.ReadFile(n)
	if err != nil {
		return err
	}

	var jsmap []map[string]interface{}
	if err := json.Unmarshal(buf, &jsmap); err != nil {
		// It may be an unconverted non-array .json, 
		// so try to read it in as a non-array.
		var smap map[string]interface{}
		if json.Unmarshal(buf, &smap) != nil {
			return err
		}
		jsmap = append(jsmap, smap)

	}
	if config.Debug {
		log.Printf("%v: %v", n, jsmap)
	}

	for _, j := range jsmap {
		log.Printf("Fixup %v\n", j)
		for _, d := range f {
			d(n, j)
		}
		log.Printf("Done Fixup %v\n", j)
		log.Printf("j[Name] is %v", j["Nmae"])
	}
	out, err := json.MarshalIndent(jsmap, "", "\t")
	if err != nil {
		return err
	}
	out = append(out, '\n')
	if config.Debug {
		os.Stdout.Write(out)
	} else {
		ioutil.WriteFile(n, out, 0666)
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
				if config.Except[fi.Name()] {
					return filepath.SkipDir
				}
				return nil
			}
			n := fi.Name()
			if len(n) < 5 || n[len(n)-5:] != ".json" {
				return nil
			}
			todo := []fixup{cflags, removeempty, checkname}
			// For now, don't do this. But I'd like it to come back as we
			// had some issues with Cflags.
			if true || config.Except[n] {
				todo = todo[1:]
			}
			log.Printf("process %s", name)
			err = one(name, todo...)
			if err != nil {
				log.Printf("%s: %s\n", name, err)
				return err
			}
			return nil
		})

		if err != nil {
			log.Fatalf("%v", err)

		}
	}
}
