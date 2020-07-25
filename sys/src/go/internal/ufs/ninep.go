// Copyright 2012 The Ninep Authors.  All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.
// This code is imported from the old ninep repo,
// with some changes.

package ufs

import (
	"flag"
	"os"

	"github.com/Harvey-OS/ninep/protocol"
)

var (
	user = flag.String("user", "harvey", "Default user name")
)

func modeToUnixFlags(mode protocol.Mode) int {
	ret := int(0)
	switch mode & 3 {
	case protocol.OREAD:
		ret = os.O_RDONLY
		break

	case protocol.ORDWR:
		ret = os.O_RDWR
		break

	case protocol.OWRITE:
		ret = os.O_WRONLY
		break

	case protocol.OEXEC:
		ret = os.O_RDONLY
		break
	}

	if mode&protocol.OTRUNC != 0 {
		ret |= os.O_TRUNC
	}

	return ret
}

func dirToQIDType(d os.FileInfo) uint8 {
	ret := uint8(0)
	if d.IsDir() {
		ret |= protocol.QTDIR
	}

	if d.Mode()&os.ModeSymlink != 0 {
		ret |= protocol.QTSYMLINK
	}

	return ret
}

func dirTo9p2000Mode(d os.FileInfo) uint32 {
	ret := uint32(d.Mode() & 0777)
	if d.IsDir() {
		ret |= protocol.DMDIR
	}
	return ret
}

func dirTo9p2000Dir(fi os.FileInfo) (*protocol.Dir, error) {
	d := &protocol.Dir{}
	d.QID = fileInfoToQID(fi)
	d.Mode = dirTo9p2000Mode(fi)
	// TODO: use info on systems that have it.
	d.Atime = uint32(fi.ModTime().Unix()) // uint32(atime(sysMode).Unix())
	d.Mtime = uint32(fi.ModTime().Unix())
	d.Length = uint64(fi.Size())
	d.Name = fi.Name()
	d.User = *user
	d.Group = *user

	return d, nil
}
