// +build !harvey

package main

import (
	"log"
	"os"
	"os/exec"
	"path"
	"strings"
)

func init() {
	harvey = os.Getenv("HARVEY")
	if harvey != "" {
		return
	}
	out, err := exec.Command("git", "rev-parse", "--show-toplevel").Output()
	failOn(err)
	harvey = strings.TrimSpace(string(out))
	if harvey == "" {
		log.Fatal("Set the HARVEY environment variable or run from a git checkout.")
	}

	os.Setenv("PATH", strings.Join([]string{fromRoot("/util"), os.Getenv("PATH")}, string(os.PathListSeparator)))
}

// return the given absolute path as an absolute path rooted at the harvey tree.
func fromRoot(p string) string {
	p = os.ExpandEnv(p)
	if path.IsAbs(p) {
		return path.Join(harvey, p)
	}
	return p
}
