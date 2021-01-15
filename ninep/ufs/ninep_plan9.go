// Copyright 2021 The Ninep Authors.  All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.
// This code is imported from the old ninep repo,
// with some changes.

package ufs

import (
	"os"
	"syscall"

	"harvey-os.org/ninep/protocol"
)

func fileInfoToQID(d os.FileInfo) protocol.QID {
	if stat, ok := d.Sys().(*syscall.Dir); ok {
		return protocol.QID{
			Path:    stat.Qid.Path,
			Version: stat.Qid.Vers,
			Type:    stat.Qid.Type,
		}
	}
	return protocol.QID{
		Path:    uint64(d.ModTime().UnixNano()),
		Version: uint32(d.ModTime().UnixNano() / 1000000),
		Type:    dirToQIDType(d),
	}
}
