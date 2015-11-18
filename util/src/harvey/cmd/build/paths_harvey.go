// +build harvey

package main

import (
	"os"
	"strings"
)

func init() {
	harvey = os.Getenv("HARVEY")
	if harvey != "" {
		return
	}
	harvey = "/"
	os.Setenv("path", strings.Join([]string{fromRoot("/util"), os.Getenv("path")}, string(os.PathListSeparator)))
}
