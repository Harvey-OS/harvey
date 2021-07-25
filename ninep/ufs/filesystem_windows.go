// Copyright 2012 The Ninep Authors.  All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.
// This code is imported from the old ninep repo,
// with some changes.

// +build windows

package ufs

import "os"

// resetDir closes the underlying file and reopens it so it can be read again.
// This is because Windows doesn't seem to support calling Seek on a directory
// handle.
func resetDir(f *File) error {
	f2, err := os.OpenFile(f.fullName, os.O_RDONLY, 0)
	if err != nil {
		return err
	}

	f.File.Close()
	f.File = f2
	return nil
}
