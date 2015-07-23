package main

import (
	"fmt"
	"log"
	"os"
	"os/exec"
	"path"
	"strings"
)

func findTools(toolprefix string) (err error) {
	for k, v := range tools {
		if x := os.Getenv(strings.ToUpper(k)); x != "" {
			v = x
		}
		v = toolprefix + v
		v, err = exec.LookPath(v)
		if err != nil {
			return err
		}
		tools[k] = v
	}
	return nil
}

func mkcmd(exe string, args ...string) *exec.Cmd {
	cmd := exec.Command(exe, args...)
	fmt.Println("running", cmd.Args)
	cmd.Stdin = os.Stdin
	cmd.Stderr = os.Stderr
	cmd.Stdout = os.Stdout
	return cmd
}

func fail(err error) {
	if err != nil {
		log.New(os.Stderr, "", log.LstdFlags|log.Lshortfile).Output(2, err.Error())
		os.Exit(1)
	}
}

func adjust1(s string) string {
	if path.IsAbs(s) {
		return path.Join(harvey, s)
	}
	return s
}

func adjust(s []string) (r []string) {
	for _, v := range s {
		r = append(r, adjust1(v))
	}
	return
}
