package main

import (
	"bytes"
	"flag"
	"fmt"
	"io"
	"log"
	"net"
	"sync"

	"github.com/Harvey-OS/go/internal/tmpfs"
	"github.com/Harvey-OS/ninep/pkg/debugfs"
	"github.com/Harvey-OS/ninep/protocol"
)

var (
	networktype = flag.String("ntype", "tcp4", "Default network type")
	netaddr     = flag.String("addr", ":5641", "Network address")
	debug       = flag.Int("debug", 0, "Print debug messages")
)

type fileServer struct {
	Versioned bool

	archive    *tmpfs.Archive
	filesMutex sync.Mutex
	files      map[protocol.FID]tmpfs.Entry
	ioUnit     protocol.MaxSize
}

func (fs *fileServer) Rversion(msize protocol.MaxSize, version string) (protocol.MaxSize, string, error) {
	if version != "9P2000" {
		return 0, "", fmt.Errorf("%v not supported; only 9P2000", version)
	}
	return msize, version, nil
}

// TODO handle aname?
func (fs *fileServer) Rattach(fid protocol.FID, afid protocol.FID, uname string, aname string) (protocol.QID, error) {
	if afid != protocol.NOFID {
		return protocol.QID{}, fmt.Errorf("We don't do auth attach")
	}

	// There should be no .. or other such junk in the Aname. Clean it up anyway.
	/*aname = path.Join("/", aname)
	st, err := os.Stat(aname)
	if err != nil {
		return protocol.QID{}, err
	}*/

	root := fs.archive.Root()
	fs.filesMutex.Lock()
	fs.files[fid] = root
	fs.filesMutex.Unlock()

	return root.Qid(), nil
}

func (fs *fileServer) Rflush(o protocol.Tag) error {
	return nil
}

func (fs *fileServer) Rwalk(fid protocol.FID, newfid protocol.FID, paths []string) ([]protocol.QID, error) {
	// Lookup the parent fid
	parentEntry, err := fs.getFile(fid)
	if err != nil {
		return nil, fmt.Errorf("does not exist")
	}

	if len(paths) == 0 {
		// Clone fid - point to same entry
		fs.filesMutex.Lock()
		defer fs.filesMutex.Unlock()
		_, ok := fs.files[newfid]
		if ok {
			return nil, fmt.Errorf("FID in use: clone walk, fid %d newfid %d", fid, newfid)
		}

		fs.files[newfid] = parentEntry
		return []protocol.QID{}, nil
	}

	walkQids := make([]protocol.QID, len(paths))

	var i int
	var pathcmp string
	currEntry := parentEntry
	for i, pathcmp = range paths {
		currEntry, ok := currEntry.Child(pathcmp)
		if !ok {
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
			return walkQids[:i], nil
		}
		walkQids[i] = currEntry.Qid()
	}

	fs.filesMutex.Lock()
	defer fs.filesMutex.Unlock()
	// this is quite unlikely, which is why we don't bother checking for it first.
	if fid != newfid {
		if _, ok := fs.files[newfid]; ok {
			return nil, fmt.Errorf("FID in use: walk to %v, fid %v, newfid %v", paths, fid, newfid)
		}
	}
	fs.files[newfid] = currEntry
	return walkQids, nil
}

func (fs *fileServer) Ropen(fid protocol.FID, mode protocol.Mode) (protocol.QID, protocol.MaxSize, error) {
	// Lookup the parent fid
	f, err := fs.getFile(fid)
	if err != nil {
		return protocol.QID{}, 0, fmt.Errorf("does not exist")
	}

	return f.Qid(), fs.ioUnit, nil
}

func (fs *fileServer) Rcreate(fid protocol.FID, name string, perm protocol.Perm, mode protocol.Mode) (protocol.QID, protocol.MaxSize, error) {
	return protocol.QID{}, 0, fmt.Errorf("Filesystem is read-only")
}

func (fs *fileServer) Rclunk(fid protocol.FID) error {
	_, err := fs.clunk(fid)
	return err
}

func (fs *fileServer) Rstat(fid protocol.FID) ([]byte, error) {
	f, err := fs.getFile(fid)
	if err != nil {
		return []byte{}, err
	}
	d := f.P9Dir()
	var b bytes.Buffer
	protocol.Marshaldir(&b, *d)
	return b.Bytes(), nil
}

func (fs *fileServer) Rwstat(fid protocol.FID, b []byte) error {
	return fmt.Errorf("Filesystem is read-only")
}

func (fs *fileServer) Rremove(fid protocol.FID) error {
	return fmt.Errorf("Filesystem is read-only")
}

func (fs *fileServer) Rread(fid protocol.FID, o protocol.Offset, c protocol.Count) ([]byte, error) {
	f, err := fs.getFile(fid)
	if err != nil {
		return nil, err
	}

	dataReader := bytes.NewReader(f.P9Data().Bytes())
	b := make([]byte, c)
	n, err := dataReader.ReadAt(b, int64(o))
	if err != nil && err != io.EOF {
		return nil, err
	}
	return b[:n], nil
}

func (fs *fileServer) Rwrite(fid protocol.FID, o protocol.Offset, b []byte) (protocol.Count, error) {
	return -1, fmt.Errorf("Filesystem is read-only")
}

func (fs *fileServer) getFile(fid protocol.FID) (tmpfs.Entry, error) {
	fs.filesMutex.Lock()
	defer fs.filesMutex.Unlock()

	f, ok := fs.files[fid]
	if !ok {
		return nil, fmt.Errorf("does not exist")
	}
	return f, nil
}

func (fs *fileServer) clunk(fid protocol.FID) (tmpfs.Entry, error) {
	fs.filesMutex.Lock()
	defer fs.filesMutex.Unlock()
	f, ok := fs.files[fid]
	if !ok {
		return nil, fmt.Errorf("does not exist")
	}
	delete(fs.files, fid)

	return f, nil
}

func newTmpfs(arch *tmpfs.Archive, opts ...protocol.ListenerOpt) (*protocol.Listener, error) {
	if arch == nil {
		return nil, fmt.Errorf("No archive")
	}

	nsCreator := func() protocol.NineServer {
		fs := &fileServer{archive: arch}
		fs.files = make(map[protocol.FID]tmpfs.Entry)
		fs.ioUnit = 8192

		var ns protocol.NineServer = fs
		if *debug != 0 {
			ns = &debugfs.DebugFileServer{FileServer: fs}
		}
		return ns
	}

	l, err := protocol.NewListener(nsCreator, opts...)
	if err != nil {
		return nil, err
	}
	return l, nil
}

func main() {
	flag.Parse()

	// TODO replace with loading from cmd line
	buf := tmpfs.CreateTestImage()
	arch := tmpfs.ReadImage(buf)

	// Bind and listen on the socket.
	listener, err := net.Listen(*networktype, *netaddr)
	if err != nil {
		log.Fatalf("Listen failed: %v", err)
	}

	ufslistener, err := newTmpfs(arch, func(l *protocol.Listener) error {
		l.Trace = nil // log.Printf
		return nil
	})

	if err := ufslistener.Serve(listener); err != nil {
		log.Fatal(err)
	}
}
