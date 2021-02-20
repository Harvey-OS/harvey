// Program 9ipfs is a 9P file server for the InterPlanetary File System (IPFS).
//
// After mounting the file system at a mount point mtpt, files and
// directories from IPFS are accessed from mtpt/CID, where CID is the
// IPFS content identifier.
package main

import (
	"bytes"
	"context"
	"errors"
	"flag"
	"fmt"
	"io"
	"io/ioutil"
	"log"
	"net"
	"os"
	"sync"

	icore "github.com/ipfs/interface-go-ipfs-core"
	"harvey-os.org/cmd/9ipfs/internal/ipfs"
	"harvey-os.org/ninep"
	"harvey-os.org/ninep/protocol"
	"harvey-os.org/sys"
)

var (
	srv         = flag.String("s", "9ipfs", "service name")
	debug       = flag.Bool("debug", false, "print debug messages")
	network     = flag.String("net", "tcp", "network used to listen")
	addr        = flag.String("addr", "", "network address to listen on")
	tempRepo    = flag.Bool("tmp", true, "use a temporary IPFS repository")
	tempRepoDir = flag.String("tmpdir", "", "override temporary IPFS repository directory with this one")
)

// These errors try to match those used in Plan 9. See:
//	/sys/src/lib9p/srv.c
//	/sys/src/9/port/error.h
var (
	ErrUnknownFID   = errors.New("unknown fid")
	ErrPermission   = errors.New("permission denied")
	ErrFileNotFound = errors.New("file not found")
	ErrDuplicateFID = errors.New("duplicate fid")
	ErrBadOffset    = errors.New("bad offset")
	ErrWalkNoDir    = errors.New("walk in non-directory")
)

func getQid(e *ipfs.DirEntry) protocol.QID {
	t := uint8(protocol.QTFILE)
	switch e.Type {
	case icore.TSymlink, icore.TFile:
		t = protocol.QTFILE
	case icore.TDirectory:
		t = protocol.QTDIR
	}
	// First few bytes of the CID usually encodes the type of hash,
	// so use last 64-bits of the CID to build the path.
	b := e.Cid.Bytes()
	path := uint64(0)
	i := len(b) - 8
	if i < 0 {
		i = 0
	}
	for _, bb := range b[i:] {
		path = (path << 8) | uint64(bb)
	}
	return protocol.QID{Type: t, Version: 0, Path: path}
}

func plan9Dir(e *ipfs.DirEntry) *protocol.Dir {
	mode := uint32(0555)
	if e.Type == icore.TDirectory {
		mode |= protocol.DMDIR
	}
	return &protocol.Dir{
		QID:     getQid(e),
		Mode:    mode,
		Atime:   0,
		Mtime:   0,
		Length:  e.Size,
		Name:    e.Name,
		User:    "ipfs",
		Group:   "ipfs",
		ModUser: "ipfs",
	}
}

type fileServer struct {
	sync.Mutex

	ioUnit protocol.MaxSize
	ipfs   *ipfs.IPFS
	files  map[protocol.FID]*ipfs.DirEntry
}

func newFileServer(ipfsNode *ipfs.IPFS) *fileServer {
	return &fileServer{
		ioUnit: 1 * 1024 * 1024,
		ipfs:   ipfsNode,
		files:  make(map[protocol.FID]*ipfs.DirEntry),
	}
}

// Rversion initiates the session
func (fs *fileServer) Rversion(msize protocol.MaxSize, version string) (protocol.MaxSize, string, error) {
	if version != "9P2000" {
		return 0, "unknown", nil
	}
	return msize, version, nil
}

// Rattach attaches a fid to the root for the given user.  aname and afid are not used.
func (fs *fileServer) Rattach(fid protocol.FID, afid protocol.FID, uname string, aname string) (protocol.QID, error) {
	if afid != protocol.NOFID {
		return protocol.QID{}, ErrUnknownFID
	}

	fs.setFile(fid, fs.ipfs.Root())
	return getQid(fs.ipfs.Root()), nil
}

// Rflush does nothing.
func (fs *fileServer) Rflush(o protocol.Tag) error {
	return nil
}

// Rwalk walks the hierarchy from fid, with the walk path determined by paths
func (fs *fileServer) Rwalk(fid protocol.FID, newfid protocol.FID, names []string) ([]protocol.QID, error) {
	ctx := context.Background()

	// Lookup the parent fid
	parent, err := fs.getFile(fid)
	if err != nil {
		return nil, ErrFileNotFound
	}

	if len(names) == 0 {
		// Clone fid - point to same entry
		fs.setFile(newfid, parent)
		return []protocol.QID{}, nil
	}
	if parent.Type != icore.TDirectory {
		return nil, ErrWalkNoDir
	}

	qids := make([]protocol.QID, len(names))

	curr := parent
	for i, name := range names {
		if name == ".." {
			curr = curr.Parent
		} else {
			curr, err = fs.ipfs.Walk(ctx, curr, name)
			if err != nil {
				if i == 0 {
					// If the first element cannot be walked, return Rerror.
					return nil, err
				}
				// Otherwise return whatever much we have so far,
				// but newfid is not affected.
				return qids[:i], nil
			}
		}
		qids[i] = getQid(curr)
	}

	fs.setFile(newfid, curr)
	return qids, nil
}

// Ropen opens the file associated with fid
func (fs *fileServer) Ropen(fid protocol.FID, mode protocol.Mode) (protocol.QID, protocol.MaxSize, error) {
	if mode&(protocol.OTRUNC|protocol.ORCLOSE|protocol.OAPPEND) != 0 {
		return protocol.QID{}, 0, ErrPermission
	}
	switch mode & 3 {
	case protocol.OWRITE, protocol.ORDWR:
		return protocol.QID{}, 0, ErrPermission
	}

	// Lookup the parent fid
	f, err := fs.getFile(fid)
	if err != nil {
		return protocol.QID{}, 0, ErrFileNotFound
	}

	// TODO Check executable

	return getQid(f), fs.ioUnit, nil
}

// Rcreate not supported since it's a read-only filesystem
func (fs *fileServer) Rcreate(fid protocol.FID, name string, perm protocol.Perm, mode protocol.Mode) (protocol.QID, protocol.MaxSize, error) {
	return protocol.QID{}, 0, ErrPermission
}

// Rclunk drops the fid association in the file system
func (fs *fileServer) Rclunk(fid protocol.FID) error {
	_, err := fs.clunk(fid)
	return err
}

// Rstat returns stat message for the file associated with fid
func (fs *fileServer) Rstat(fid protocol.FID) ([]byte, error) {
	f, err := fs.getFile(fid)
	if err != nil {
		return []byte{}, err
	}

	d := plan9Dir(f)

	var b bytes.Buffer
	protocol.Marshaldir(&b, *d)
	return b.Bytes(), nil
}

// Rwstat not supported since it's a read-only filesystem
func (fs *fileServer) Rwstat(fid protocol.FID, b []byte) error {
	return ErrPermission
}

// Rremove not supported since it's a read-only filesystem
func (fs *fileServer) Rremove(fid protocol.FID) error {
	return ErrPermission
}

// Rread returns up to c bytes from file fid starting at offset o
func (fs *fileServer) Rread(fid protocol.FID, offset protocol.Offset, count protocol.Count) ([]byte, error) {
	f, err := fs.getFile(fid)
	if err != nil {
		return nil, err
	}

	ctx := context.Background()

	switch f.Type {
	case icore.TDirectory:
		entries, err := fs.ipfs.ReadDir(ctx, f)
		if err != nil {
			return nil, err
		}

		start := 0
		for start < int(offset) && len(entries) > 0 {
			var discard bytes.Buffer
			protocol.Marshaldir(&discard, *plan9Dir(&entries[0]))
			start += discard.Len()
			entries = entries[1:]
		}
		if len(entries) == 0 {
			return nil, nil
		}
		if start != int(offset) {
			return nil, ErrBadOffset
		}

		var b bytes.Buffer
		for b.Len() < int(count) && len(entries) > 0 {
			var tmp bytes.Buffer
			protocol.Marshaldir(&tmp, *plan9Dir(&entries[0]))
			entries = entries[1:]

			if b.Len()+tmp.Len() > int(count) {
				break
			}
			b.Write(tmp.Bytes())
		}
		return b.Bytes(), nil

	case icore.TFile:
		file, err := fs.ipfs.GetFile(ctx, f)
		if err != nil {
			return nil, err
		}
		defer file.Close()
		if _, err := file.Seek(int64(offset), 0); err != nil {
			return nil, err
		}
		b := make([]byte, count)
		n, err := file.Read(b)
		if err != nil && err != io.EOF {
			return nil, err
		}
		return b[:n], nil
	}
	log.Printf("fid %v file type %v is unknown", fid, f.Type)
	return nil, nil
}

// Rwrite not supported since it's a read-only filesystem
func (fs *fileServer) Rwrite(fid protocol.FID, o protocol.Offset, b []byte) (protocol.Count, error) {
	return 0, ErrPermission
}

func (fs *fileServer) getFile(fid protocol.FID) (*ipfs.DirEntry, error) {
	fs.Lock()
	defer fs.Unlock()

	f, ok := fs.files[fid]
	if !ok {
		return nil, ErrFileNotFound
	}
	return f, nil
}

// Associate newfid with the entry, assuming newfid isn't already in use
func (fs *fileServer) setFile(newfid protocol.FID, entry *ipfs.DirEntry) error {
	fs.Lock()
	defer fs.Unlock()

	if _, ok := fs.files[newfid]; ok {
		return ErrDuplicateFID
	}
	fs.files[newfid] = entry
	return nil
}

func (fs *fileServer) clunk(fid protocol.FID) (*ipfs.DirEntry, error) {
	fs.Lock()
	defer fs.Unlock()

	f, ok := fs.files[fid]
	if !ok {
		return nil, ErrUnknownFID
	}
	delete(fs.files, fid)

	return f, nil
}

var usage = func() {
	fmt.Fprintf(flag.CommandLine.Output(), "Usage: %v [options]\n", os.Args[0])
	flag.PrintDefaults()
	os.Exit(1)
}

func main() {
	flag.Usage = usage
	flag.Parse()

	ctx, cancel := context.WithCancel(context.Background())
	defer cancel()

	if *tempRepoDir == "" {
		d, err := ioutil.TempDir("", "9ipfs")
		if err != nil {
			log.Fatal(err)
		}
		*tempRepoDir = d
		// User is responsible for removing the directory if they provide it.
		defer os.RemoveAll(*tempRepoDir)
	}

	ipfsNode, err := ipfs.New(ctx, &ipfs.Config{
		LogWriter:   os.Stderr,
		TempRepo:    *tempRepo,
		TempRepoDir: *tempRepoDir,
	})
	if err != nil {
		log.Fatal(err)
	}

	nsCreator := func() protocol.NineServer {
		fs := newFileServer(ipfsNode)
		if *debug {
			return &ninep.DebugFileServer{FileServer: fs}
		}
		return fs
	}
	if *addr != "" {
		ln, err := net.Listen(*network, *addr)
		if err != nil {
			log.Fatal(err)
		}
		netListener, err := protocol.NewNetListener(nsCreator)
		if err != nil {
			log.Fatal(err)
		}
		if err := netListener.Serve(ln); err != nil {
			log.Fatal(err)
		}
	} else {
		fd, err := sys.PostPipe(*srv)
		if err != nil {
			log.Fatal(err)
		}
		protocol.ServeFromRWC(os.NewFile(uintptr(fd), *srv), nsCreator(), *srv)
	}
}
