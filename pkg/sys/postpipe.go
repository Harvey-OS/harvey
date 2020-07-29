// Copyright 2020 The Ninep Authors.  All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

package sys

import (
	"fmt"
	"io/ioutil"
	"os"
	"path/filepath"
	"syscall"
)

// PostPipe creates a pipe, posts one side of it
// #s at 'n', and returns the other side.
func PostPipe(n string) (int, error) {
	var fd [2]int
	err := syscall.Pipe(fd[:])

	if err != nil {
		return -1, err
	}
	// We could, possibly, use os.Args[0] save that the caller may have
	// rewritten or even removed them.
	if len(n) == 0 {
		n = fmt.Sprintf("post.%d", os.Getpid())
	}
	n = filepath.Join("#s", n)
	if err := ioutil.WriteFile(n, []byte(fmt.Sprintf("%d", fd[1])), 0644); err != nil {
		return -1, err
	}
	return fd[0], nil
}
