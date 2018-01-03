// Copyright (C) 2018 Harvey OS
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

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
