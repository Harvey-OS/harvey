package main

import (
	"archive/tar"
	"bytes"
	"compress/gzip"
	"log"
	"testing"

	"github.com/Harvey-OS/go/internal/tmpfs"
)

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

func Test_newTmpfs(t *testing.T) {
	fs, err := newTmpfs(nil)
	if fs != nil || err == nil {
		t.Fatal("no archive - should have failed")
	}

	arch := tmpfs.ReadImage(createTestImage())

	fs, err = newTmpfs(arch)
	if fs == nil || err != nil {
		t.Fatal("should not have failed")
	}
}
