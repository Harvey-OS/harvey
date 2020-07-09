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
	qid() protocol.QID
}

type file struct {
	hdr     *tar.Header
	data    *bytes.Buffer
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

type directory struct {
	dirFullName string
	entries     map[string]entry
	dirQid      protocol.QID
}

func newDirectory(fullName string) *directory {
	return &directory{dirFullName: fullName, entries: map[string]entry{}, dirQid: protocol.QID{}}
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
		newDir := newDirectory(strings.Join(cmps, "/"))
		d.entries[cmpname] = newDir

		newDir.dirQid.Type = protocol.QTFILE
		newDir.dirQid.Path = uint64(len(a.dirs))

		a.dirs = append(a.dirs, newDir)

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

	fs := &archive{newDirectory(""), []*directory{}, []*file{}, time.Now()}
	tr := tar.NewReader(gzr)
	for {
		hdr, err := tr.Next()
		if err == io.EOF {
			break // End of archive
		}
		if err != nil {
			log.Fatal(err)
		}

		filename := hdr.Name
		file := newFile(hdr)
		if _, err := io.Copy(file.data, tr); err != nil {
			log.Fatal(err)
		}

		fs.addFile(filename, file)
	}

	return fs
}
