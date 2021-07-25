// Copyright 2009 The Ninep Authors.  All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

package ufs

import (
	"bytes"
	"fmt"
	"io"
	"log"
	"os"
	"path"
	"path/filepath"
	"sync"
	"time"

	"harvey-os.org/go/pkg/ninep"
	"harvey-os.org/go/pkg/ninep/protocol"
)

type file struct {
	protocol.QID
	fullName string
	file     *os.File
	// We can't know how big a serialized dentry is until we serialize it.
	// At that point it might be too big. We save it here if that happens,
	// and on the next directory read we start with that.
	oflow []byte
}

type FileServer struct {
	root      *file
	rootPath  string
	Versioned bool
	IOunit    protocol.MaxSize

	// mu guards below
	mu    sync.Mutex
	files map[protocol.FID]*file
}

func stat(s string) (*protocol.Dir, protocol.QID, error) {
	var q protocol.QID
	st, err := os.Lstat(s)
	if err != nil {
		return nil, q, fmt.Errorf("does not exist")
	}
	d, err := dirTo9p2000Dir(st)
	if err != nil {
		return nil, q, nil
	}
	q = fileInfoToQID(st)
	return d, q, nil
}

func (e *FileServer) Rversion(msize protocol.MaxSize, version string) (protocol.MaxSize, string, error) {
	if version != "9P2000" {
		return 0, "", fmt.Errorf("%v not supported; only 9P2000", version)
	}
	e.Versioned = true
	return msize, version, nil
}

func (e *FileServer) getFile(fid protocol.FID) (*file, error) {
	e.mu.Lock()
	defer e.mu.Unlock()
	f, ok := e.files[fid]
	if !ok {
		return nil, fmt.Errorf("does not exist")
	}

	return f, nil
}

func (e *FileServer) Rattach(fid protocol.FID, afid protocol.FID, uname string, aname string) (protocol.QID, error) {
	if afid != protocol.NOFID {
		return protocol.QID{}, fmt.Errorf("We don't do auth attach")
	}
	// There should be no .. or other such junk in the Aname. Clean it up anyway.
	aname = path.Join("/", aname)
	aname = path.Join(e.rootPath, aname)
	st, err := os.Stat(aname)
	if err != nil {
		return protocol.QID{}, err
	}
	r := &file{fullName: aname}
	r.QID = fileInfoToQID(st)
	e.files[fid] = r
	e.root = r
	return r.QID, nil
}

func (e *FileServer) Rflush(o protocol.Tag) error {
	return nil
}

func (e *FileServer) Rwalk(fid protocol.FID, newfid protocol.FID, paths []string) ([]protocol.QID, error) {
	e.mu.Lock()
	f, ok := e.files[fid]
	e.mu.Unlock()
	if !ok {
		return nil, fmt.Errorf("does not exist")
	}
	if len(paths) == 0 {
		e.mu.Lock()
		defer e.mu.Unlock()
		_, ok := e.files[newfid]
		if ok {
			return nil, fmt.Errorf("FID in use: clone walk, fid %d newfid %d", fid, newfid)
		}
		nf := *f
		e.files[newfid] = &nf
		return []protocol.QID{}, nil
	}
	p := f.fullName
	q := make([]protocol.QID, len(paths))

	var i int
	for i = range paths {
		p = path.Join(p, paths[i])
		st, err := os.Lstat(p)
		if err != nil {
			// From the RFC: If the first element cannot be walked for any
			// reason, Rerror is returned. Otherwise, the walk will return an
			// Rwalk message containing nwqid qids corresponding, in order, to
			// the files that are visited by the nwqid successful elementwise
			// walks; nwqid is therefore either nwname or the index of the
			// first elementwise walk that failed. The value of nwqid cannot be
			// zero unless nwname is zero. Also, nwqid will always be less than
			// or equal to nwname. Only if it is equal, however, will newfid be
			// affected, in which case newfid will represent the file reached
			// by the final elementwise walk requested in the message.
			//
			// to sum up: if any walks have succeeded, you return the QIDS for
			// one more than the last successful walk
			if i == 0 {
				return nil, fmt.Errorf("file does not exist")
			}
			// we only get here if i is > 0 and less than nwname,
			// so the i should be safe.
			return q[:i], nil
		}
		q[i] = fileInfoToQID(st)
	}
	e.mu.Lock()
	defer e.mu.Unlock()
	// this is quite unlikely, which is why we don't bother checking for it first.
	if fid != newfid {
		if _, ok := e.files[newfid]; ok {
			return nil, fmt.Errorf("FID in use: walk to %v, fid %v, newfid %v", paths, fid, newfid)
		}
	}
	e.files[newfid] = &file{fullName: p, QID: q[i]}
	return q, nil
}

func (e *FileServer) Ropen(fid protocol.FID, mode protocol.Mode) (protocol.QID, protocol.MaxSize, error) {
	e.mu.Lock()
	f, ok := e.files[fid]
	e.mu.Unlock()
	if !ok {
		return protocol.QID{}, 0, fmt.Errorf("does not exist")
	}

	var err error
	f.file, err = os.OpenFile(f.fullName, modeToUnixFlags(mode), 0)
	if err != nil {
		return protocol.QID{}, 0, err
	}

	return f.QID, e.IOunit, nil
}
func (e *FileServer) Rcreate(fid protocol.FID, name string, perm protocol.Perm, mode protocol.Mode) (protocol.QID, protocol.MaxSize, error) {
	f, err := e.getFile(fid)
	if err != nil {
		return protocol.QID{}, 0, err
	}
	if f.file != nil {
		return protocol.QID{}, 0, fmt.Errorf("FID already open")
	}
	n := path.Join(f.fullName, name)
	if perm&protocol.Perm(protocol.DMDIR) != 0 {
		p := os.FileMode(int(perm) & 0777)
		err := os.Mkdir(n, p)
		_, q, err := stat(n)
		if err != nil {
			return protocol.QID{}, 0, err
		}
		f.fullName = n
		f.QID = q
		return q, 8000, err
	}

	m := modeToUnixFlags(mode) | os.O_CREATE | os.O_TRUNC
	p := os.FileMode(perm) & 0777
	of, err := os.OpenFile(n, m, p)
	if err != nil {
		return protocol.QID{}, 0, err
	}
	_, q, err := stat(n)
	if err != nil {
		return protocol.QID{}, 0, err
	}
	f.fullName = n
	f.QID = q
	f.file = of
	return q, 8000, err
}
func (e *FileServer) Rclunk(fid protocol.FID) error {
	_, err := e.clunk(fid)
	return err
}

func (e *FileServer) Rstat(fid protocol.FID) ([]byte, error) {
	f, err := e.getFile(fid)
	if err != nil {
		return []byte{}, err
	}
	st, err := os.Lstat(f.fullName)
	if err != nil {
		return []byte{}, fmt.Errorf("ENOENT")
	}
	d, err := dirTo9p2000Dir(st)
	if err != nil {
		return []byte{}, nil
	}
	var b bytes.Buffer
	protocol.Marshaldir(&b, *d)
	return b.Bytes(), nil
}
func (e *FileServer) Rwstat(fid protocol.FID, b []byte) error {
	var changed bool
	f, err := e.getFile(fid)
	if err != nil {
		return err
	}
	dir, err := protocol.Unmarshaldir(bytes.NewBuffer(b))
	if err != nil {
		return err
	}
	if dir.Mode != 0xFFFFFFFF {
		changed = true
		mode := dir.Mode & 0777
		if err := os.Chmod(f.fullName, os.FileMode(mode)); err != nil {
			return err
		}
	}

	// Try to find local uid, gid by name.
	if dir.User != "" || dir.Group != "" {
		return fmt.Errorf("Permission denied")
		changed = true
	}

	/*
		if uid != ninep.NOUID || gid != ninep.NOUID {
			changed = true
			e := os.Chown(fid.path, int(uid), int(gid))
			if e != nil {
				req.RespondError(toError(e))
				return
			}
		}
	*/

	if dir.Name != "" {
		changed = true
		// If we path.Join dir.Name to / before adding it to
		// the fid path, that ensures nobody gets to walk out of the
		// root of this server.
		newname := path.Join(path.Dir(f.fullName), path.Join("/", dir.Name))

		// absolute renaming. Ufs can do this, so let's support it.
		// We'll allow an absolute path in the Name and, if it is,
		// we will make it relative to root. This is a gigantic performance
		// improvement in systems that allow it.
		if filepath.IsAbs(dir.Name) {
			newname = path.Join(e.rootPath, dir.Name)
		}

		// If to exists, and to is a directory, we can't do the
		// rename, since os.Rename will move from into to.

		st, err := os.Stat(newname)
		if err == nil && st.IsDir() {
			return fmt.Errorf("is a directory")
		}
		if err := os.Rename(f.fullName, newname); err != nil {
			return err
		}
		f.fullName = newname
	}

	if dir.Length != 0xFFFFFFFFFFFFFFFF {
		changed = true
		if err := os.Truncate(f.fullName, int64(dir.Length)); err != nil {
			return err
		}
	}

	// If either mtime or atime need to be changed, then
	// we must change both.
	if dir.Mtime != ^uint32(0) || dir.Atime != ^uint32(0) {
		changed = true
		mt, at := time.Unix(int64(dir.Mtime), 0), time.Unix(int64(dir.Atime), 0)
		if cmt, cat := (dir.Mtime == ^uint32(0)), (dir.Atime == ^uint32(0)); cmt || cat {
			st, err := os.Stat(f.fullName)
			if err != nil {
				return err
			}
			switch cmt {
			case true:
				mt = st.ModTime()
			default:
				//at = atime(st.Sys().(*syscall.Stat_t))
			}
		}
		if err := os.Chtimes(f.fullName, at, mt); err != nil {
			return err
		}
	}

	if !changed && f.file != nil {
		f.file.Sync()
	}
	return nil
}

func (e *FileServer) clunk(fid protocol.FID) (*file, error) {
	e.mu.Lock()
	defer e.mu.Unlock()
	f, ok := e.files[fid]
	if !ok {
		return nil, fmt.Errorf("does not exist")
	}
	delete(e.files, fid)
	// What do we do if we can't close it?
	// All I can think of is to log it.
	if f.file != nil {
		if err := f.file.Close(); err != nil {
			log.Printf("Close of %v failed: %v", f.fullName, err)
		}
	}
	return f, nil
}

// Rremove removes the file. The question of whether the file continues to be accessible
// is system dependent.
func (e *FileServer) Rremove(fid protocol.FID) error {
	f, err := e.clunk(fid)
	if err != nil {
		return err
	}
	return os.Remove(f.fullName)
}

func (e *FileServer) Rread(fid protocol.FID, o protocol.Offset, c protocol.Count) ([]byte, error) {
	f, err := e.getFile(fid)
	if err != nil {
		return nil, err
	}
	if f.file == nil {
		return nil, fmt.Errorf("FID not open")
	}
	if f.QID.Type&protocol.QTDIR != 0 {
		if o == 0 {
			f.oflow = nil
			if err := resetDir(f); err != nil {
				return nil, err
			}
		}

		// We make the assumption that they can always fit at least one
		// directory entry into a read. If that assumption does not hold
		// so many things are broken that we can't fix them here.
		// But we'll drop out of the loop below having returned nothing
		// anyway.
		b := bytes.NewBuffer(f.oflow)
		f.oflow = nil
		pos := 0

		for {
			if b.Len() > int(c) {
				f.oflow = b.Bytes()[pos:]
				return b.Bytes()[:pos], nil
			}
			pos += b.Len()
			st, err := f.file.Readdir(1)
			if err == io.EOF {
				return b.Bytes(), nil
			}
			if err != nil {
				return nil, err
			}

			d9p, err := dirTo9p2000Dir(st[0])
			if err != nil {
				return nil, err
			}
			protocol.Marshaldir(b, *d9p)
			// Seen on linux clients: sometimes the math is wrong and
			// they end up asking for the last element with not enough data.
			// Linux bug or bug with this server? Not sure yet.
			if b.Len() > int(c) {
				log.Printf("Warning: Server bug? %v, need %d bytes;count is %d: skipping", d9p, b.Len(), c)
				return nil, nil
			}
			// We're not quite doing the array right.
			// What does work is returning one thing so, for now, do that.
			return b.Bytes(), nil
		}
		return b.Bytes(), nil
	}

	// N.B. even if they ask for 0 bytes on some file systems it is important to pass
	// through a zero byte read (not Unix, of course).
	b := make([]byte, c)
	n, err := f.file.ReadAt(b, int64(o))
	if err != nil && err != io.EOF {
		return nil, err
	}
	return b[:n], nil
}

func (e *FileServer) Rwrite(fid protocol.FID, o protocol.Offset, b []byte) (protocol.Count, error) {
	f, err := e.getFile(fid)
	if err != nil {
		return -1, err
	}
	if f.file == nil {
		return -1, fmt.Errorf("FID not open")
	}

	// N.B. even if they ask for 0 bytes on some file systems it is important to pass
	// through a zero byte write (not Unix, of course). Also, let the underlying file system
	// manage the error if the open mode was wrong. No need to duplicate the logic.

	n, err := f.file.WriteAt(b, int64(o))
	return protocol.Count(n), err
}

func NewUFS(root string, debug int, opts ...protocol.ListenerOpt) (*protocol.Listener, error) {
	nsCreator := func() protocol.NineServer {
		f := &FileServer{}
		f.files = make(map[protocol.FID]*file)
		f.rootPath = root // for now.
		f.IOunit = 8192

		// any opts for the ufs layer can be added here too ...
		var d protocol.NineServer = f
		if debug != 0 {
			d = &ninep.DebugFileServer{FileServer: f}
		}
		return d
	}

	l, err := protocol.NewListener(nsCreator, opts...)
	if err != nil {
		return nil, err
	}
	return l, nil
}
