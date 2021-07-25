package tmpfs

import (
	"archive/tar"
	"bytes"
	"compress/gzip"
	"fmt"
	"io"
	"log"
	"strings"
	"time"

	"github.com/Harvey-OS/ninep/protocol"
)

type Archive struct {
	root     *directory
	dirs     []*directory
	files    []*file
	openTime time.Time
}

type Entry interface {
	FullName() string
	Child(name string) (Entry, bool)

	// p9 server qid
	Qid() protocol.QID

	// p9 dir structure (stat)
	P9Dir() *protocol.Dir

	// p9 data representation
	P9Data() *bytes.Buffer
}

type file struct {
	hdr     *tar.Header
	data    *bytes.Buffer // TODO Should this be []byte
	fileQid protocol.QID
}

func newFile(hdr *tar.Header, id uint64) *file {
	return &file{
		hdr:     hdr,
		data:    &bytes.Buffer{},
		fileQid: protocol.QID{Type: protocol.QTFILE, Version: 0, Path: id},
	}
}

func (f *file) FullName() string {
	return f.hdr.Name
}

func (f *file) Child(name string) (Entry, bool) {
	return nil, false
}

func (f *file) Qid() protocol.QID {
	return f.fileQid
}

func (f *file) P9Dir() *protocol.Dir {
	d := &protocol.Dir{}
	d.QID = f.fileQid
	d.Mode = uint32(f.hdr.Mode) & 0777
	d.Atime = uint32(f.hdr.AccessTime.Unix())
	d.Mtime = uint32(f.hdr.ModTime.Unix())
	d.Length = uint64(f.hdr.Size)
	d.Name = f.FullName()
	d.User = f.hdr.Uname
	d.Group = f.hdr.Gname
	return d
}

func (f *file) P9Data() *bytes.Buffer {
	return f.data
}

type directory struct {
	dirFullName string
	entries     map[string]Entry
	data        *bytes.Buffer // TODO Should this be []byte
	dirQid      protocol.QID
	openTime    time.Time
}

func newDirectory(fullName string, openTime time.Time, id uint64) *directory {
	return &directory{
		dirFullName: fullName,
		entries:     map[string]Entry{},
		data:        &bytes.Buffer{},
		dirQid:      protocol.QID{Type: protocol.QTDIR, Version: 0, Path: id},
		openTime:    openTime}
}

func (d *directory) FullName() string {
	return d.dirFullName
}

func (d *directory) Child(name string) (Entry, bool) {
	e, ok := d.entries[name]
	return e, ok
}

func (d *directory) Qid() protocol.QID {
	return d.dirQid
}

func (d *directory) P9Dir() *protocol.Dir {
	pd := &protocol.Dir{}
	pd.QID = d.dirQid
	pd.Mode = 0444
	pd.Atime = uint32(d.openTime.Unix())
	pd.Mtime = uint32(d.openTime.Unix())
	pd.Length = 4096
	pd.Name = d.FullName()
	pd.User = ""
	pd.Group = ""
	return pd
}

func (d *directory) P9Data() *bytes.Buffer {
	return d.data
}

func (a *Archive) Root() *directory {
	return a.root
}

func (a *Archive) addFile(filepath string, file *file) error {
	filecmps := strings.Split(filepath, "/")
	if dir, err := a.getOrCreateDir(a.root, filecmps[:len(filecmps)-1]); err != nil {
		return err
	} else {
		return a.createFile(dir, filecmps[len(filecmps)-1], file)
	}
}

func (a *Archive) getOrCreateDir(d *directory, cmps []string) (*directory, error) {
	if len(cmps) == 0 {
		return d, nil
	}

	cmpname := cmps[0]
	if entry, exists := d.entries[cmpname]; exists {
		if dir, ok := entry.(*directory); ok {
			return a.getOrCreateDir(dir, cmps[1:])
		} else {
			return nil, fmt.Errorf("File already exists with name %s", cmpname)
		}
	} else {
		newDir := newDirectory(strings.Join(cmps, "/"), a.openTime, uint64(len(a.dirs)))
		a.dirs = append(a.dirs, newDir)

		// Add the child dir to the parent
		// Also serialize in p9 marshalled form so we don't need to faff around in Rread
		d.entries[cmpname] = newDir
		protocol.Marshaldir(d.data, *newDir.P9Dir())

		return a.getOrCreateDir(newDir, cmps[1:])
	}
}

func (a *Archive) createFile(d *directory, filename string, file *file) error {
	if _, exists := d.entries[filename]; exists {
		return fmt.Errorf("File or directory already exists with name %s", filename)
	}
	d.entries[filename] = file

	file.fileQid.Type = protocol.QTFILE
	file.fileQid.Path = uint64(len(a.files) + 1)

	a.files = append(a.files, file)
	protocol.Marshaldir(d.data, *file.P9Dir())

	return nil
}

// Create and add some files to the archive.
func CreateTestImageTemp() *bytes.Buffer {
	var buf bytes.Buffer

	gztw := gzip.NewWriter(&buf)
	defer func() {
		if err := gztw.Close(); err != nil {
			log.Fatal(err)
		}
	}()

	tw := tar.NewWriter(gztw)
	defer func() {
		if err := tw.Close(); err != nil {
			log.Fatal(err)
		}
	}()

	var files = []struct {
		Name, Body string
	}{
		{"readme.txt", "This archive contains some text files."},
		{"foo/gopher.txt", "Gopher names:\nGeorge\nGeoffrey\nGonzo"},
		{"bar/todo.txt", "Get animal handling license."},
		{"foo/todo2.txt", "harvey lalal"},
		{"abc/123/sean.txt", "lorem ipshum."},
	}
	for _, file := range files {
		hdr := &tar.Header{
			Name: file.Name,
			Mode: 0600,
			Size: int64(len(file.Body)),
		}
		if err := tw.WriteHeader(hdr); err != nil {
			log.Fatal(err)
		}
		if _, err := tw.Write([]byte(file.Body)); err != nil {
			log.Fatal(err)
		}
	}

	return &buf
}

// Read a compressed tar and produce a file hierarchy
func ReadImage(buf *bytes.Buffer) *Archive {
	gzr, err := gzip.NewReader(buf)
	if err != nil {
		log.Fatal(err)
	}
	defer func() {
		if err := gzr.Close(); err != nil {
			log.Fatal(err)
		}
	}()

	openTime := time.Now()
	fs := &Archive{newDirectory("", openTime, 0), []*directory{}, []*file{}, openTime}
	tr := tar.NewReader(gzr)
	for id := 0; ; id++ {
		hdr, err := tr.Next()
		if err == io.EOF {
			break // End of archive
		}
		if err != nil {
			log.Fatal(err)
		}

		file := newFile(hdr, uint64(id))
		if _, err := io.Copy(file.data, tr); err != nil {
			log.Fatal(err)
		}

		fs.addFile(hdr.Name, file)
	}

	return fs
}
