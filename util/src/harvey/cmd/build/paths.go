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
	if harvey == "" {
		// Try figuring it out from git instead
		out, err := exec.Command("git", "rev-parse", "--show-toplevel").Output()
		if err == nil {
			harvey = strings.TrimSpace(string(out))
		}
	}
	// If we still haven't found the path, bail out.
	if harvey == "" {
		log.Fatal("Set the HARVEY environment variable or run from a git checkout.")
	}

	os.Setenv("PATH", strings.Join([]string{fromRoot("/util"), os.Getenv("PATH")}, string(os.PathListSeparator)))
}
