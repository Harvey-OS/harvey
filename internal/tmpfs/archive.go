package tmpfs

import (
	"archive/tar"
	"compress/gzip"
	"fmt"
	"io"
	"log"
	"path"
	"strings"
	"time"

	"github.com/Harvey-OS/ninep/protocol"
)

type Archive struct {
	root     *Directory   // root directory
	dirs     []*Directory // array of all directories (idx is qid)
	files    []*File      // array of all files (idx is qid)
	openTime time.Time
}

type Entry interface {
	Name() string

	// p9 server qid
	Qid() protocol.QID

	// p9 dir structure (stat)
	P9Dir(uname string) *protocol.Dir
}

type File struct {
	name    string
	hdr     *tar.Header
	data    []byte
	fileQid protocol.QID
}

func newFile(hdr *tar.Header, id uint64, dataSize uint64) *File {
	return &File{
		name:    path.Base(hdr.Name),
		hdr:     hdr,
		data:    make([]byte, dataSize),
		fileQid: protocol.QID{Type: protocol.QTFILE, Version: 0, Path: id},
	}
}

func (f *File) Name() string {
	return f.name
}

func (f *File) Qid() protocol.QID {
	return f.fileQid
}

func (f *File) P9Dir(uname string) *protocol.Dir {
	d := &protocol.Dir{}
	d.QID = f.fileQid
	d.Mode = uint32(f.hdr.Mode) & 0777
	d.Atime = uint32(f.hdr.AccessTime.Unix())
	d.Mtime = uint32(f.hdr.ModTime.Unix())
	d.Length = uint64(f.hdr.Size)
	d.Name = f.Name()
	d.User = uname
	d.Group = uname
	return d
}

func (f *File) Data() []byte {
	return f.data
}

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

func (d *Directory) Name() string {
	return d.name
}

func (d *Directory) ChildByName(name string) (Entry, bool) {
	e, ok := d.nameToEntryMap[name]
	return e, ok
}

func (d *Directory) Child(i int) Entry {
	return d.entries[i]
}

func (d *Directory) Parent() Entry {
	return d.parent
}

func (d *Directory) NumChildren() int {
	return len(d.entries)
}

func (d *Directory) Qid() protocol.QID {
	return d.dirQid
}

func (d *Directory) P9Dir(uname string) *protocol.Dir {
	pd := &protocol.Dir{}
	pd.QID = d.dirQid
	pd.Mode = 0444
	pd.Atime = uint32(d.openTime.Unix())
	pd.Mtime = uint32(d.openTime.Unix())
	pd.Length = 4096
	pd.Name = d.Name()
	pd.User = uname
	pd.Group = uname
	return pd
}

func (a *Archive) Root() *Directory {
	return a.root
}

func (a *Archive) addFile(filepath string, file *File) error {
	filecmps := strings.Split(filepath, "/")
	//fmt.Println(filecmps)

	isFile := !file.hdr.FileInfo().IsDir()
	var dirCmps []string
	if isFile {
		dirCmps = filecmps[:len(filecmps)-1]
	} else {
		dirCmps = filecmps
	}

	dir, err := a.getOrCreateDir(a.root, dirCmps)
	if err != nil {
		return err
	}

	if isFile {
		return a.createFile(dir, filecmps[len(filecmps)-1], file)
	}
	return nil
}

func (a *Archive) getOrCreateDir(d *Directory, cmps []string) (*Directory, error) {
	if len(cmps) == 0 {
		return d, nil
	}
	//fmt.Println(cmps)

	cmpname := cmps[0]
	if cmpname == "" {
		return d, nil
	}

	if entry, exists := d.nameToEntryMap[cmpname]; exists {
		if dir, ok := entry.(*Directory); ok {
			return a.getOrCreateDir(dir, cmps[1:])
		} else {
			return nil, fmt.Errorf("File already exists with name %s", cmpname)
		}
	} else {
		newDir := newDirectory(cmpname, d, a.openTime, uint64(len(a.dirs)))
		fmt.Printf("Adding newDirectory: %v cmps: %v\n", newDir.name, cmpname)
		a.dirs = append(a.dirs, newDir)

		// Add the child dir to the parent
		// Also serialize in p9 marshalled form so we don't need to faff around in Rread
		d.nameToEntryMap[cmpname] = newDir
		d.entries = append(d.entries, newDir)

		return a.getOrCreateDir(newDir, cmps[1:])
	}
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

func (a *Archive) DumpEntry(e Entry, parentPath string) {
	if dir, isDir := e.(*Directory); isDir {
		parentPath = path.Join(parentPath, dir.name)
		for _, child := range dir.entries {
			a.DumpEntry(child, parentPath)
		}
	} else if file, isFile := e.(*File); isFile {
		log.Printf("%v\n", path.Join(parentPath, file.name))
	}
}

func (a *Archive) DumpArchive() {
	a.DumpEntry(a.root, "")
}

// ReadImage reads a compressed tar to produce a file hierarchy
func ReadImage(r io.Reader) *Archive {
	gzr, err := gzip.NewReader(r)
	if err != nil {
		log.Fatal(err)
	}
	defer func() {
		if err := gzr.Close(); err != nil {
			log.Fatal(err)
		}
	}()

	openTime := time.Now()
	fs := &Archive{newDirectory("", nil, openTime, 0), []*Directory{}, []*File{}, openTime}
	tr := tar.NewReader(gzr)
	for id := 0; ; id++ {
		hdr, err := tr.Next()
		if err == io.EOF {
			break // End of archive
		}
		if err != nil {
			log.Fatal(err)
		}

		//fmt.Println(hdr.Name)
		file := newFile(hdr, uint64(id), uint64(hdr.Size))
		if _, err := io.ReadFull(tr, file.data); err != nil {
			log.Fatal(err)
		}

		if err = fs.addFile(hdr.Name, file); err != nil {
			log.Fatal(err)
		}
	}

	return fs
}
