// +build !harvey

package main

import (
	"log"
	"os"
	"os/exec"
	"strings"
)

func init() {
	harvey = os.Getenv("HARVEY")
	// git is purely optional, for lazy people.
	out, err := exec.Command("git", "rev-parse", "--show-toplevel").Output()
	if err == nil {
		harvey = strings.TrimSpace(string(out))
	}
	if harvey == "" {
		log.Fatal("Set the HARVEY environment variable or run from a git checkout.")
	}

	os.Setenv("PATH", strings.Join([]string{fromRoot("/util"), os.Getenv("PATH")}, string(os.PathListSeparator)))
}
