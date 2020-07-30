package main

import (
	"archive/tar"
	"bytes"
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
func createTestImage(t *testing.T, base int) *bytes.Buffer {
	var buf bytes.Buffer

	tw := tar.NewWriter(&buf)
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
			Size: int64(base + len(file.Body)),
		}
		t.Logf("Write header for %v", hdr)
		if err := tw.WriteHeader(hdr); err != nil {
			log.Fatal(err)
		}
		t.Logf("Wrote header for %v", hdr)
		if _, err := tw.Write([]byte(append(make([]byte, base), []byte(file.Body)...))); err != nil {
			log.Fatal(err)
		}
		t.Logf("Wrote body for %v", hdr)
		t.Logf("file is now %#x", buf.Len())
	}

	return &buf
}

func TestReadImage(t *testing.T) {
	tmpfs.Debug = t.Logf
	_, err := tmpfs.ReadImageTar(createTestImage(t, 0))
	if err != nil {
		t.Fatal(err)
	}
}

func TestReadImageBig(t *testing.T) {
	tmpfs.Debug = t.Logf
	_, err := tmpfs.ReadImageTar(createTestImage(t, 1048576))
	if err != nil {
		t.Fatal(err)
	}
}
