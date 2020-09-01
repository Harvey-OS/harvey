// +build go1.14

package main

import (
	"bytes"
	"flag"
	"fmt"
	"io"
	"io/ioutil"
	"log"
	"os"
	"path/filepath"
	"sync"

	"github.com/ulikunitz/xz/lzma"
	"harvey-os.org/ninep/protocol"
	"harvey-os.org/ninep/tmpfs"
	"harvey-os.org/sys"
)

var (
	srv   = flag.String("s", "tmpfs", "srv name")
	debug = flag.Bool("d", false, "Print debug messages")
	ext   = flag.String("ext", "cpio", "file type")
)

// Constant error messages to match those found in the linux 9p source.
// These strings are used by linux to map to particular error codes.
// (See net/9p/error.c in the Linux code)
const (
	ErrorAuthFailed   = "authentication failed"
	ErrorReadOnlyFs   = "Read-only file system"
	ErrorFileNotFound = "file not found"
	ErrorFidInUse     = "fid already in use"
	ErrorFidNotFound  = "fid unknown or out of range"
)

type fileServer struct {
	sync.Mutex

	Versioned bool

	ioUnit  protocol.MaxSize
	archive *tmpfs.Archive
	files   map[protocol.FID]*FidEntry
}

// FidEntry wraps an Entry with the instance data required for a fid reference
type FidEntry struct {
	tmpfs.Entry

	// Username to be used for all entries in this hierarchy
	uname string

	// We can't know how big a serialized dentry is until we serialize it.
	// At that point it might be too big. We save it here if that happens,
	// and on the next directory read we start with that.
	oflow []byte

	// Index of next child to return when reading a directory
	nextChildIdx int
}

func newFidEntry(entry tmpfs.Entry, uname string) *FidEntry {
	return &FidEntry{Entry: entry, uname: uname, oflow: nil, nextChildIdx: -1}
}

// Rversion initiates the session
func (fs *fileServer) Rversion(msize protocol.MaxSize, version string) (protocol.MaxSize, string, error) {
	if version != "9P2000" {
		return 0, "", fmt.Errorf("%v not supported; only 9P2000", version)
	}
	return msize, version, nil
}

// Rattach attaches a fid to the root for the given user.  aname and afid are not used.
func (fs *fileServer) Rattach(fid protocol.FID, afid protocol.FID, uname string, aname string) (protocol.QID, error) {
	if afid != protocol.NOFID {
		return protocol.QID{}, fmt.Errorf(ErrorAuthFailed)
	}

	root := fs.archive.Root()
	fs.setFile(fid, root, uname)

	return root.Qid(), nil
}

// Rflush does nothing in tmpfs
func (fs *fileServer) Rflush(o protocol.Tag) error {
	return nil
}

// Rwalk walks the hierarchy from fid, with the walk path determined by paths
func (fs *fileServer) Rwalk(fid protocol.FID, newfid protocol.FID, paths []string) ([]protocol.QID, error) {
	// Lookup the parent fid
	parentEntry, err := fs.getFile(fid)
	if err != nil {
		return nil, fmt.Errorf(ErrorFileNotFound)
	}

	if len(paths) == 0 {
		// Clone fid - point to same entry
		fs.setFile(newfid, parentEntry.Entry, parentEntry.uname)
		return []protocol.QID{}, nil
	}

	walkQids := make([]protocol.QID, len(paths))

	currEntry := parentEntry.Entry
	for i, pathcmp := range paths {
		var ok bool
		var dir *tmpfs.Directory
		if dir, ok = currEntry.(*tmpfs.Directory); ok {
			if pathcmp == ".." {
				currEntry = dir.Parent()
			} else {
				currEntry, err = dir.ChildByName(pathcmp)
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
						return nil, err
					}
					// we only get here if i is > 0 and less than nwname,
					// so the i should be safe.
					return walkQids[:i], nil
				}
			}
		}
		walkQids[i] = currEntry.Qid()
	}

	fs.setFile(newfid, currEntry, parentEntry.uname)
	return walkQids, nil
}

// Ropen opens the file associated with fid
func (fs *fileServer) Ropen(fid protocol.FID, mode protocol.Mode) (protocol.QID, protocol.MaxSize, error) {
	if mode&(protocol.OTRUNC|protocol.ORCLOSE|protocol.OAPPEND) != 0 {
		return protocol.QID{}, 0, fmt.Errorf(ErrorReadOnlyFs)
	}
	switch mode & 3 {
	case protocol.OWRITE, protocol.ORDWR:
		return protocol.QID{}, 0, fmt.Errorf(ErrorReadOnlyFs)
	}

	// Lookup the parent fid
	f, err := fs.getFile(fid)
	if err != nil {
		return protocol.QID{}, 0, fmt.Errorf(ErrorFileNotFound)
	}

	// TODO Check executable

	return f.Qid(), fs.ioUnit, nil
}

// Rcreate not supported since it's a read-only filesystem
func (fs *fileServer) Rcreate(fid protocol.FID, name string, perm protocol.Perm, mode protocol.Mode) (protocol.QID, protocol.MaxSize, error) {
	return protocol.QID{}, 0, fmt.Errorf(ErrorReadOnlyFs)
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

	d := f.P9Dir(f.uname)

	var b bytes.Buffer
	protocol.Marshaldir(&b, *d)
	return b.Bytes(), nil
}

// Rwstat not supported since it's a read-only filesystem
func (fs *fileServer) Rwstat(fid protocol.FID, b []byte) error {
	return fmt.Errorf(ErrorReadOnlyFs)
}

// Rremove not supported since it's a read-only filesystem
func (fs *fileServer) Rremove(fid protocol.FID) error {
	return fmt.Errorf(ErrorReadOnlyFs)
}

// Rread returns up to c bytes from file fid starting at offset o
func (fs *fileServer) Rread(fid protocol.FID, o protocol.Offset, c protocol.Count) ([]byte, error) {
	f, err := fs.getFile(fid)
	if err != nil {
		return nil, err
	}

	if dir, ok := f.Entry.(*tmpfs.Directory); ok {
		if o == 0 {
			f.oflow = nil
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

			f.nextChildIdx++
			pos += b.Len()

			if f.nextChildIdx >= dir.NumChildren() {
				return b.Bytes(), nil
			}
			d9p := dir.Child(f.nextChildIdx).P9Dir(f.uname)
			protocol.Marshaldir(b, *d9p)

			// Seen on linux clients: sometimes the math is wrong and
			// they end up asking for the last element with not enough data.
			// Linux bug or bug with this server? Not sure yet.
			if b.Len() > int(c) {
				log.Printf("Warning: Server bug? %v, need %d bytes;count is %d: skipping", d9p, b.Len(), c)
				return nil, nil
			}
			// TODO handle more than one entry at a time
			// We're not quite doing the array right.
			// What does work is returning one thing so, for now, do that.
			return b.Bytes(), nil
		}

	} else if file, ok := f.Entry.(*tmpfs.File); ok {
		end := int(o) + int(c)
		maxEnd := len(file.Data())
		if end > maxEnd {
			end = maxEnd
		}
		return file.Data()[o:end], nil
	}
	log.Fatalf("Unrecognised FidEntry")
	return nil, nil
}

// Rwrite not supported since it's a read-only filesystem
func (fs *fileServer) Rwrite(fid protocol.FID, o protocol.Offset, b []byte) (protocol.Count, error) {
	return -1, fmt.Errorf(ErrorReadOnlyFs)
}

func (fs *fileServer) getFile(fid protocol.FID) (*FidEntry, error) {
	fs.Lock()
	defer fs.Unlock()

	f, ok := fs.files[fid]
	if !ok {
		return nil, fmt.Errorf(ErrorFileNotFound)
	}
	return f, nil
}

// Associate newfid with the entry, assuming newfid isn't already in use
func (fs *fileServer) setFile(newfid protocol.FID, entry tmpfs.Entry, uname string) error {
	fs.Lock()
	defer fs.Unlock()
	if _, ok := fs.files[newfid]; ok {
		return fmt.Errorf(ErrorFidInUse)
	}
	fs.files[newfid] = newFidEntry(entry, uname)
	return nil
}

func (fs *fileServer) clunk(fid protocol.FID) (tmpfs.Entry, error) {
	fs.Lock()
	defer fs.Unlock()

	f, ok := fs.files[fid]
	if !ok {
		return nil, fmt.Errorf(ErrorFidNotFound)
	}
	delete(fs.files, fid)

	return f, nil
}

var usage = func() {
	fmt.Fprintf(flag.CommandLine.Output(), "Usage: tmpfs [options] <tarfile>\n")
	flag.PrintDefaults()
	os.Exit(1)
}

func readImage(n string) (*tmpfs.Archive, error) {
	f, err := os.Open(n)
	if err != nil {
		log.Fatal(err)
	}
	defer func() {
		if err := f.Close(); err != nil {
			log.Print(err)
		}
	}()
	r := io.Reader(f)

	if arch, err := lzma.NewReader(f); err == nil {
		r = arch
	} else {
		if _, err := f.Seek(0, 0); err != nil {
			return nil, err
		}
	}

	if len(*ext) == 0 {
		*ext = filepath.Ext(n)
	}
	if *ext == "tgz" || *ext == "gz" {
		return tmpfs.ReadImageTar(r)
	} else if *ext == "cpio" {
		// This is where Unix people say "but mmap!!!"
		b, err := ioutil.ReadAll(r)
		if err != nil {
			return nil, err
		}
		return tmpfs.ReadImageCpio(bytes.NewReader(b))
	}
	return nil, fmt.Errorf("No reader matches suffix %s", *ext)
}

func main() {
	flag.Usage = usage
	flag.Parse()

	a := flag.Args()
	// TODO: serve more than one archive.
	if len(a) != 1 {
		flag.Usage()
	}
	if *debug {
		protocol.Debug = log.Printf
		tmpfs.Debug = log.Printf
	}

	arch, err := readImage(a[0])
	if err != nil {
		log.Fatal(err)
	}
	if *debug {
		arch.DumpArchive()
	}
	fd, err := sys.PostPipe(*srv)
	if err != nil {
		log.Fatal(err)
	}

	fs := &fileServer{archive: arch, files: make(map[protocol.FID]*FidEntry), ioUnit: 1 * 1024 * 1024}

	// TODO: get the tracing back in.
	// The ninep package was from a long time ago and it's
	// awkward at best.
	protocol.ServeFromRWC(os.NewFile(uintptr(fd), *srv), fs, *srv)
}
