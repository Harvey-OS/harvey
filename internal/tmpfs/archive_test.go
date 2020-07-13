package tmpfs

import (
	"archive/tar"
	"bytes"
	"compress/gzip"
	"log"
	"testing"
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

func TestReadArchive(t *testing.T) {
	arch := ReadImage(createTestImage())

	// Read root
	root := arch.Root()
	if root.FullName() != "" {
		t.Fatal("incorrect root name")
	}

	// Read readme.txt
	readme, ok := root.Child("readme.txt")
	if !ok {
		t.Fatal("couldn't open readme.txt")
	}
	readmeData := readme.P9Data().String()
	if readmeData != "This archive contains some text files." {
		t.Fatalf("readme.txt didn't contain expected string - found '%s'", readmeData)
	}

	// Read abc/123/sean.txt
	abc, ok := root.Child("abc")
	if !ok {
		t.Fatal("couldn't open abc")
	}
	dir123, ok := abc.Child("123")
	if !ok {
		t.Fatal("couldn't open 123")
	}
	sean, ok := dir123.Child("sean.txt")
	if !ok {
		t.Fatal("couldn't open sean.txt")
	}
	seanData := sean.P9Data().String()
	if seanData != "lorem ipshum." {
		t.Fatalf("sean.txt didn't contain expected string - found '%s'", seanData)
	}
}
