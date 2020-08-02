package main

import (
	"archive/tar"
	"bytes"
	"compress/gzip"
	"fmt"
	"log"
	"testing"

	"harvey-os.org/ninep/tmpfs"
)

// Lots of tests shamelessly lifted from the ninep/ufs code

func print(f string, args ...interface{}) {
	fmt.Printf(f+"\n", args...)
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
		{"foo/bar/hello.txt", "hello world!"},
	}
	for _, file := range files {
		hdr := &tar.Header{
			Name: file.Name,
			Mode: 0600,
			Size: int64(1048576 + len(file.Body)),
		}
		if err := tw.WriteHeader(hdr); err != nil {
			log.Fatal(err)
		}
		if _, err := tw.Write([]byte(append(make([]byte, 1048576), []byte(file.Body)...))); err != nil {
			log.Fatal(err)
		}
	}

	return &buf
}

func TestReadImage(t *testing.T) {
	_, err := tmpfs.ReadImage(createTestImage())
	if err != nil {
		t.Fatal(err)
	}
}
