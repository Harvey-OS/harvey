package main

import (
	"archive/tar"
	"bytes"
	"compress/gzip"
	"fmt"
	"io"
	"log"
	"strings"
	"time"

	"github.com/harvey-os/ninep/protocol"
)

type archive struct {
	root     *directory
	dirs     []*directory
	files    []*file
	openTime time.Time
}

type entry interface {
	fullName() string
	child(name string) (entry, bool)

	// p9 server qid
	qid() protocol.QID

	// p9 dir structure (stat)
	p9Dir() *protocol.Dir

	// p9 data representation
	p9Data() *bytes.Buffer
}

type file struct {
	hdr     *tar.Header
	data    *bytes.Buffer // TODO Should this be []byte
	fileQid protocol.QID
}

func newFile(hdr *tar.Header) *file {
	return &file{
		hdr:     hdr,
		data:    &bytes.Buffer{},
		fileQid: protocol.QID{},
	}
}

func (f *file) fullName() string {
	return f.hdr.Name
}

func (f *file) child(name string) (entry, bool) {
	return nil, false
}

func (f *file) qid() protocol.QID {
	return f.fileQid
}

func (f *file) p9Dir() *protocol.Dir {
	d := &protocol.Dir{}
	d.QID = f.fileQid
	d.Mode = uint32(f.hdr.Mode) & 0777
	d.Atime = uint32(f.hdr.AccessTime.Unix())
	d.Mtime = uint32(f.hdr.ModTime.Unix())
	d.Length = uint64(f.hdr.Size)
	d.Name = f.fullName()
	d.User = f.hdr.Uname
	d.Group = f.hdr.Gname
	return d
}

func (f *file) p9Data() *bytes.Buffer {
	return f.data
}

type directory struct {
	dirFullName string
	entries     map[string]entry
	data        *bytes.Buffer // TODO Should this be []byte
	dirQid      protocol.QID
	openTime    time.Time
}

func newDirectory(fullName string, openTime time.Time) *directory {
	return &directory{
		dirFullName: fullName,
		entries:     map[string]entry{},
		data:        &bytes.Buffer{},
		dirQid:      protocol.QID{},
		openTime:    openTime}
}

func (d *directory) fullName() string {
	return d.dirFullName
}

func (d *directory) child(name string) (entry, bool) {
	e, ok := d.entries[name]
	return e, ok
}

func (d *directory) qid() protocol.QID {
	return d.dirQid
}

func (d *directory) p9Dir() *protocol.Dir {
	pd := &protocol.Dir{}
	pd.QID = d.dirQid
	pd.Mode = 0444
	pd.Atime = uint32(d.openTime.Unix())
	pd.Mtime = uint32(d.openTime.Unix())
	pd.Length = 4096
	pd.Name = d.fullName()
	pd.User = ""
	pd.Group = ""
	return pd
}

func (d *directory) p9Data() *bytes.Buffer {
	return d.data
}

func (a *archive) addFile(filepath string, file *file) error {
	filecmps := strings.Split(filepath, "/")
	if dir, err := a.getOrCreateDir(a.root, filecmps[:len(filecmps)-1]); err != nil {
		return err
	} else {
		return a.createFile(dir, filecmps[len(filecmps)-1], file)
	}
}

func (a *archive) getOrCreateDir(d *directory, cmps []string) (*directory, error) {
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
		newDir := newDirectory(strings.Join(cmps, "/"), a.openTime)
		newDir.dirQid.Type = protocol.QTFILE
		newDir.dirQid.Path = uint64(len(a.dirs))

		a.dirs = append(a.dirs, newDir)

		// Add the child dir to the parent
		// Also serialize in p9 marshalled form so we don't need to faff around in Rread
		d.entries[cmpname] = newDir
		protocol.Marshaldir(newDir.data, *newDir.p9Dir())

		return a.getOrCreateDir(newDir, cmps[1:])
	}
}

func (a *archive) createFile(d *directory, filename string, file *file) error {
	if _, exists := d.entries[filename]; exists {
		return fmt.Errorf("File or directory already exists with name %s", filename)
	}
	d.entries[filename] = file

	file.fileQid.Type = protocol.QTDIR
	file.fileQid.Path = uint64(len(a.files))

	a.files = append(a.files, file)

	return nil
}

// Create and add some files to the archive.
func createTestImage() *bytes.Buffer {
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
func readImage(buf *bytes.Buffer) *archive {
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
	fs := &archive{newDirectory("", openTime), []*directory{}, []*file{}, openTime}
	tr := tar.NewReader(gzr)
	for {
		hdr, err := tr.Next()
		if err == io.EOF {
			break // End of archive
		}
		if err != nil {
			log.Fatal(err)
		}

		file := newFile(hdr)
		if _, err := io.Copy(file.data, tr); err != nil {
			log.Fatal(err)
		}

		fs.addFile(hdr.Name, file)
	}

	return fs
}
