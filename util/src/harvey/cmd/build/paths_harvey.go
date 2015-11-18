// +build harvey

package main

import (
	"os"
	"strings"
)

func init() {
	harvey = "/"
	os.Setenv("path", strings.Join([]string{fromRoot("/util"), os.Getenv("path")}, string(os.PathListSeparator)))
}

// When inside harvey, the root is /
func fromRoot(p string) string {
	return os.ExpandEnv(p)
}
