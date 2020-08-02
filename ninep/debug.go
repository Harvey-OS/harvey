// Copyright 2009 The Ninep Authors.  All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

package ninep

import (
	"bytes"
	"log"

	"harvey-os.org/ninep/protocol"
)

type DebugFileServer struct {
	FileServer protocol.NineServer
}

func (dfs *DebugFileServer) Rversion(msize protocol.MaxSize, version string) (protocol.MaxSize, string, error) {
	log.Printf(">>> Tversion %v %v\n", msize, version)
	msize, version, err := dfs.FileServer.Rversion(msize, version)
	if err == nil {
		log.Printf("<<< Rversion %v %v\n", msize, version)
	} else {
		log.Printf("<<< Error %v\n", err)
	}
	return msize, version, err
}

func (dfs *DebugFileServer) Rattach(fid protocol.FID, afid protocol.FID, uname string, aname string) (protocol.QID, error) {
	log.Printf(">>> Tattach fid %v,  afid %v, uname %v, aname %v\n", fid, afid,
		uname, aname)
	qid, err := dfs.FileServer.Rattach(fid, afid, uname, aname)
	if err == nil {
		log.Printf("<<< Rattach %v\n", qid)
	} else {
		log.Printf("<<< Error %v\n", err)
	}
	return qid, err
}

func (dfs *DebugFileServer) Rflush(o protocol.Tag) error {
	log.Printf(">>> Tflush tag %v\n", o)
	err := dfs.FileServer.Rflush(o)
	if err == nil {
		log.Printf("<<< Rflush\n")
	} else {
		log.Printf("<<< Error %v\n", err)
	}
	return err
}

func (dfs *DebugFileServer) Rwalk(fid protocol.FID, newfid protocol.FID, paths []string) ([]protocol.QID, error) {
	log.Printf(">>> Twalk fid %v, newfid %v, paths %v\n", fid, newfid, paths)
	qid, err := dfs.FileServer.Rwalk(fid, newfid, paths)
	if err == nil {
		log.Printf("<<< Rwalk %v\n", qid)
	} else {
		log.Printf("<<< Error %v\n", err)
	}
	return qid, err
}

func (dfs *DebugFileServer) Ropen(fid protocol.FID, mode protocol.Mode) (protocol.QID, protocol.MaxSize, error) {
	log.Printf(">>> Topen fid %v, mode %v\n", fid, mode)
	qid, iounit, err := dfs.FileServer.Ropen(fid, mode)
	if err == nil {
		log.Printf("<<< Ropen %v %v\n", qid, iounit)
	} else {
		log.Printf("<<< Error %v\n", err)
	}
	return qid, iounit, err
}

func (dfs *DebugFileServer) Rcreate(fid protocol.FID, name string, perm protocol.Perm, mode protocol.Mode) (protocol.QID, protocol.MaxSize, error) {
	log.Printf(">>> Tcreate fid %v, name %v, perm %v, mode %v\n", fid, name,
		perm, mode)
	qid, iounit, err := dfs.FileServer.Rcreate(fid, name, perm, mode)
	if err == nil {
		log.Printf("<<< Rcreate %v %v\n", qid, iounit)
	} else {
		log.Printf("<<< Error %v\n", err)
	}
	return qid, iounit, err
}

func (dfs *DebugFileServer) Rclunk(fid protocol.FID) error {
	log.Printf(">>> Tclunk fid %v\n", fid)
	err := dfs.FileServer.Rclunk(fid)
	if err == nil {
		log.Printf("<<< Rclunk\n")
	} else {
		log.Printf("<<< Error %v\n", err)
	}
	return err
}

func (dfs *DebugFileServer) Rstat(fid protocol.FID) ([]byte, error) {
	log.Printf(">>> Tstat fid %v\n", fid)
	b, err := dfs.FileServer.Rstat(fid)
	if err == nil {
		dir, _ := protocol.Unmarshaldir(bytes.NewBuffer(b))
		log.Printf("<<< Rstat %v\n", dir)
	} else {
		log.Printf("<<< Error %v\n", err)
	}
	return b, err
}

func (dfs *DebugFileServer) Rwstat(fid protocol.FID, b []byte) error {
	dir, _ := protocol.Unmarshaldir(bytes.NewBuffer(b))
	log.Printf(">>> Twstat fid %v, %v\n", fid, dir)
	err := dfs.FileServer.Rwstat(fid, b)
	if err == nil {
		log.Printf("<<< Rwstat\n")
	} else {
		log.Printf("<<< Error %v\n", err)
	}
	return err
}

func (dfs *DebugFileServer) Rremove(fid protocol.FID) error {
	log.Printf(">>> Tremove fid %v\n", fid)
	err := dfs.FileServer.Rremove(fid)
	if err == nil {
		log.Printf("<<< Rremove\n")
	} else {
		log.Printf("<<< Error %v\n", err)
	}
	return err
}

func (dfs *DebugFileServer) Rread(fid protocol.FID, o protocol.Offset, c protocol.Count) ([]byte, error) {
	log.Printf(">>> Tread fid %v, off %v, count %v\n", fid, o, c)
	b, err := dfs.FileServer.Rread(fid, o, c)
	if err == nil {
		log.Printf("<<< Rread %v\n", len(b))
	} else {
		log.Printf("<<< Error %v\n", err)
	}
	return b, err
}

func (dfs *DebugFileServer) Rwrite(fid protocol.FID, o protocol.Offset, b []byte) (protocol.Count, error) {
	log.Printf(">>> Twrite fid %v, off %v, count %v\n", fid, o, len(b))
	c, err := dfs.FileServer.Rwrite(fid, o, b)
	if err == nil {
		log.Printf("<<< Rwrite %v\n", c)
	} else {
		log.Printf("<<< Error %v\n", err)
	}
	return c, err
}
