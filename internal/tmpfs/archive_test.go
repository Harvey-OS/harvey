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
	if root.Name() != "" {
		t.Fatal("incorrect root name")
	}

	// Read readme.txt
	readme, ok := root.ChildByName("readme.txt")
	if !ok {
		t.Fatal("couldn't open readme.txt")
	}
	readmeFile := readme.(*File)
	readmeData := string(readmeFile.Data())
	if readmeData != "This archive contains some text files." {
		t.Fatalf("readme.txt didn't contain expected string - found '%s'", readmeData)
	}

	// Read abc/123/sean.txt
	abc, ok := root.ChildByName("abc")
	if !ok {
		t.Fatal("couldn't open abc")
	}
	abcDir := abc.(*Directory)
	oneTwoThree, ok := abcDir.ChildByName("123")
	if !ok {
		t.Fatal("couldn't open 123")
	}
	oneTwoThreeDir := oneTwoThree.(*Directory)
	sean, ok := oneTwoThreeDir.ChildByName("sean.txt")
	if !ok {
		t.Fatal("couldn't open sean.txt")
	}
	seanFile := sean.(*File)
	seanData := string(seanFile.Data())
	expectedData := "lorem ipshum."
	if seanData != expectedData {
		t.Fatalf("sean.txt didn't contain expected string. expected '%s', found '%s'", expectedData, seanData)
	}
}
