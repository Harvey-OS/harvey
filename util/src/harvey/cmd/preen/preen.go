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
	Args:   os.Args[1:],
	Debug:  *flag.Bool("d", true, "Enable debug prints"),
	Harvey: os.Getenv("HARVEY"),
	Except: map[string]bool{"cflags.json": true, "9": true},
}

func one(n string) error {
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
	if _, ok := jsmap["Cflags"]; ok {
		log.Printf("Deleting Cflags from %v", n)
		delete(jsmap, "Cflags")
		a := []string{"/cflags.json"}
		for _, v := range jsmap["Include"].([]interface{}) {
			a = append(a, v.(string))
		}
		jsmap["Include"] = a
	}
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

	buf, err = json.MarshalIndent(jsmap, "", "\t")
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
	for _, n := range config.Args {
		err = filepath.Walk(n, func(name string, fi os.FileInfo, err error) error {
			if err != nil {
				return err
			}
			if fi.IsDir() {
				if config.Except[n] {
					log.Printf("skipping directory %v", n)
					return filepath.SkipDir
				}
				return nil
			}
			n := fi.Name()
			if len(n) < 5 || n[len(n)-5:] != ".json" {
				return nil
			}
			if config.Except[n] {
				return nil
			}
			log.Printf("process %s", name)
			err = one(name)
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
