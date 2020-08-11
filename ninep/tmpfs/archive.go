package tmpfs

import (
	"archive/tar"
	"fmt"
	"io"
	"io/ioutil"
	"log"
	"path"
	"path/filepath"
	"strings"
	"time"

	"github.com/u-root/u-root/pkg/cpio"
	"github.com/u-root/u-root/pkg/uio"
	"harvey-os.org/ninep/protocol"
)

// Debug can be used to print debug messages.
// It is compatible with log.Printf
var Debug = func(string, ...interface{}) {}

// Archive contains the directories and files from a decompressed archive
type Archive struct {
	root     *Directory   // root directory
	dirs     []*Directory // array of all directories (idx is qid)
	files    []*File      // array of all files (idx is qid)
	openTime time.Time
}

// Entry is a common interface for file and directorys
type Entry interface {
	Name() string

	// p9 server qid
	Qid() protocol.QID

	// p9 dir structure (stat)
	P9Dir(uname string) *protocol.Dir
}

// File describes a file from a tar archive
type File struct {
	name       string
	data       []byte
	fileQid    protocol.QID
	isFile     bool
	accessTime uint32
	modTime    uint32
	// We keep track of symlinks, even if we treat
	// them as files.
	isSymlink bool
	target    string
}

func newFile(name string, id uint64, dataSize uint64, isFile bool, accessTime uint32, modTime uint32) *File {
	return &File{
		name:       name,
		data:       make([]byte, dataSize),
		fileQid:    protocol.QID{Type: protocol.QTFILE, Version: 0, Path: id},
		isFile:     isFile,
		accessTime: accessTime,
		modTime:    modTime,
	}
}

// Name of the file (no path)
func (f *File) Name() string {
	return f.name
}

// Qid for file
func (f *File) Qid() protocol.QID {
	return f.fileQid
}

// P9Dir generates the Dir message for this file
func (f *File) P9Dir(uname string) *protocol.Dir {
	d := &protocol.Dir{}
	d.QID = f.fileQid
	d.Mode = 0444
	d.Atime = f.accessTime
	d.Mtime = f.modTime
	d.Length = uint64(len(f.data))
	d.Name = f.Name()
	d.User = uname
	d.Group = uname
	return d
}

// Data returns the data for the given file
func (f *File) Data() []byte {
	return f.data
}

// Directory describes a directory from a tar
type Directory struct {
	name           string
	nameToEntryMap map[string]Entry
	entries        []Entry
	parent         Entry
	dirQid         protocol.QID
	openTime       time.Time
}

func newDirectory(name string, parent Entry, openTime time.Time, id uint64) *Directory {
	return &Directory{
		name:           name,
		nameToEntryMap: map[string]Entry{},
		entries:        []Entry{},
		parent:         parent,
		dirQid:         protocol.QID{Type: protocol.QTDIR, Version: 0, Path: id},
		openTime:       openTime,
	}
}

// Name of the directory (no path)
func (d *Directory) Name() string {
	return d.name
}

// ChildByName looks up the child in the given directory by name
func (d *Directory) ChildByName(name string) (Entry, error) {
	e, ok := d.nameToEntryMap[name]
	if !ok {
		return nil, fmt.Errorf("%q: no such file or directory", name)
	}
	return e, nil
}

// Child looks up the child in the given directory by index
func (d *Directory) Child(i int) Entry {
	return d.entries[i]
}

// Parent returns the parent entry
func (d *Directory) Parent() Entry {
	return d.parent
}

// NumChildren returns the number of children in this directory
func (d *Directory) NumChildren() int {
	return len(d.entries)
}

// Qid returns the Qid for this directory
func (d *Directory) Qid() protocol.QID {
	return d.dirQid
}

// P9Dir generates the Dir message for this directory
func (d *Directory) P9Dir(uname string) *protocol.Dir {
	pd := &protocol.Dir{}
	pd.QID = d.dirQid
	pd.Mode = 0444 | protocol.DMDIR
	pd.Atime = uint32(d.openTime.Unix())
	pd.Mtime = uint32(d.openTime.Unix())
	pd.Length = 0
	pd.Name = d.Name()
	pd.User = uname
	pd.Group = uname
	return pd
}

// Root returns the root directory for the archive
func (a *Archive) Root() *Directory {
	return a.root
}

func (a *Archive) addFile(filepath string, file *File) error {
	filecmps := strings.Split(filepath, "/")

	var dirCmps []string
	if file.isFile {
		dirCmps = filecmps[:len(filecmps)-1]
	} else {
		dirCmps = filecmps
	}

	dir, err := a.getOrCreateDir(a.root, dirCmps)
	if err != nil {
		return err
	}

	if file.isFile {
		return a.createFile(dir, filecmps[len(filecmps)-1], file)
	}
	return nil
}

func (a *Archive) getOrCreateDir(d *Directory, cmps []string) (*Directory, error) {
	if len(cmps) == 0 {
		return d, nil
	}

	cmpname := cmps[0]
	if cmpname == "" {
		return d, nil
	}

	if entry, exists := d.nameToEntryMap[cmpname]; exists {
		// this component already exists, so try to walk down the tree
		if dir, ok := entry.(*Directory); ok {
			return a.getOrCreateDir(dir, cmps[1:])
		}
		return nil, fmt.Errorf("File already exists with name %s", cmpname)
	}

	// Create a new directory
	newDir := newDirectory(cmpname, d, a.openTime, uint64(len(a.dirs)))

	a.dirs = append(a.dirs, newDir)

	// Add the child dir to the parent
	// Also serialize in p9 marshalled form so we don't need to faff around in Rread
	d.nameToEntryMap[cmpname] = newDir
	d.entries = append(d.entries, newDir)

	return a.getOrCreateDir(newDir, cmps[1:])
}

func (a *Archive) createFile(d *Directory, filename string, file *File) error {
	if _, exists := d.nameToEntryMap[filename]; exists {
		return fmt.Errorf("File or directory already exists with name %s", filename)
	}
	d.nameToEntryMap[filename] = file
	d.entries = append(d.entries, file)

	file.fileQid.Type = protocol.QTFILE
	file.fileQid.Path = uint64(len(a.files) + 1)

	a.files = append(a.files, file)

	return nil
}

// DumpEntry will write out the archive hierarchy from the given entry and parentPath
func (a *Archive) DumpEntry(e Entry, parentPath string) {
	if dir, isDir := e.(*Directory); isDir {
		parentPath = path.Join(parentPath, dir.name)
		for _, child := range dir.entries {
			a.DumpEntry(child, parentPath)
		}
	} else if file, isFile := e.(*File); isFile {
		Debug("%v\n", path.Join(parentPath, file.name))
	}
}

// DumpArchive will write out the archive hierarchy
func (a *Archive) DumpArchive() {
	a.DumpEntry(a.root, "")
}

// ReadImageTar reads a tar file to produce an archive
func ReadImageTar(r io.Reader) (*Archive, error) {
	openTime := time.Now()
	Debug("Start: %v", openTime)
	fs := &Archive{newDirectory("/", nil, openTime, 0), []*Directory{}, []*File{}, openTime}
	Debug("got an fs: %v", fs)
	tr := tar.NewReader(r)
	Debug("got a new tar reader: %v", tr)
	for id := 0; ; id++ {
		Debug("Do %d", id)
		hdr, err := tr.Next()
		Debug("read hdr %v err %v", hdr, err)
		if err == io.EOF {
			break // End of archive
		}
		if err != nil {
			return nil, err
		}

		file := newFile(
			path.Base(hdr.Name),
			uint64(id),
			uint64(hdr.Size),
			!hdr.FileInfo().IsDir(),
			uint32(hdr.AccessTime.Unix()),
			uint32(hdr.ModTime.Unix()))
		if _, err := io.ReadFull(tr, file.data); err != nil {
			return nil, err
		}

		Debug("Now add the file")
		if err = fs.addFile(hdr.Name, file); err != nil {
			return nil, err
		}
		Debug("bottom of loop")
	}

	return fs, nil
}

// ReadImageCpio reads a cpio archive to produce a file hierarchy
func ReadImageCpio(r io.ReaderAt) (*Archive, error) {
	cpioReader := cpio.Newc.Reader(r)

	openTime := time.Now()
	arch := &Archive{newDirectory("/", nil, openTime, 0), []*Directory{}, []*File{}, openTime}

	records, err := cpio.ReadAllRecords(cpioReader)
	if err != nil {
		return nil, err
	}

	type fr struct {
		f *File
		r *cpio.Record
	}
	var files = make(map[string]*File, len(records))
	for id, record := range records {
		Debug("Record %q", record)
		file := newFile(
			path.Base(record.Name),
			uint64(id),
			record.Info.FileSize,
			record.Info.FileSize > 0,
			uint32(record.Info.MTime),
			uint32(record.Info.MTime))
		if record.Info.FileSize > 0 {
			file.data, err = ioutil.ReadAll(uio.Reader(record))
			if err != nil && err != io.EOF {
				return nil, err
			}
		}
		Debug("Add record for %q: %v", record.Name, file)
		files[record.Name] = file
	}

	for _, record := range records {
		Debug("%v: check %#o is symlink %#o", record, record.Mode, cpio.S_IFLNK)
		if record.Mode&0170000 != cpio.S_IFLNK {
			continue
		}
		n := record.Name
		f := files[n]
		sl := string(f.data)
		Debug("File %q, target %q", n, sl)
		if len(sl) == 0 {
			continue
		}

		// We traverse symlinks until the first non-symlink
		// or 32 (EMLINK). If we hit EMLINK, we do nothing.
		for i := 0; i < 32; i++ {
			var r string
			// On each iteration through this loop, we act as though
			// sl is a symlink. At some point, it will not be, and
			// our computation of the next symlink will fail.
			// At that point we know we're at the termination of a chain
			// of one or more symlinks.
			// This is essentially like doing a readlink on a path.
			// It's the easiest way to find that you have a terminal path
			// and not another symlink. It's much easier than looking at stat
			// bits.
			if filepath.IsAbs(sl) {
				r, err = filepath.Rel("/", sl)
				if err != nil {
					log.Printf("Can't make abs path %q relative to '/': %v", sl, err)
					break
				}
			} else {
				r = filepath.Join(filepath.Dir(n), sl)
			}
			Debug("Checking cpio symlink, name %q, target %q", n, r)
			t, ok := files[r]
			if ok {
				f = t
				sl = r
				continue
			}
			var newf File
			newf = *f
			newf.name = filepath.Base(record.Name)
			Debug("Found it, setting file for %q to file data for %q: %v", n, sl, newf)
			files[n] = &newf
			break
		}
	}

	for _, r := range records {
		f := files[r.Name]
		Debug("Install %q, file %v", r.Name, f)
		if err := arch.addFile(r.Name, f); err != nil {
			return nil, err
		}
	}

	return arch, nil
}
