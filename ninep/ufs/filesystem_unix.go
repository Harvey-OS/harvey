// Copyright 2012 The Ninep Authors.  All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.
// This code is imported from the old ninep repo,
// with some changes.

// +build !windows

package ufs

import "io"

// resetDir seeks to the beginning of the file so that the file list can be
// read again.
func resetDir(f *file) error {
	_, err := f.file.Seek(0, io.SeekStart)
	return err
}
