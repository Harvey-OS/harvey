package tmpfs

import (
	"archive/tar"
	"bytes"
	"compress/gzip"
	"log"
	"os"
	"strings"
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
		{"emptyDir/", ""},
		{"emptyFile", ""},
		{"readme.txt", "This archive contains some text files."},
		{"foo/gopher.txt", "Gopher names:\nGeorge\nGeoffrey\nGonzo"},
		{"bar/todo.txt", "Get animal handling license."},
		{"foo/todo2.txt", "harvey lalal"},
		{"abc/123/sean.txt", "lorem ipshum."},
	}
	for _, file := range files {
		hdr := &tar.Header{}
		if strings.HasSuffix(file.Name, "/") {
			hdr.Name = file.Name[:len(file.Name)-1]
			hdr.Mode = int64(0777 | os.ModeDir)
			hdr.Size = 0
			hdr.Typeflag = tar.TypeDir
		} else {
			hdr.Name = file.Name
			hdr.Mode = 0600
			hdr.Size = int64(len(file.Body))
			hdr.Typeflag = tar.TypeReg
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
	//arch.DumpArchive()

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
		t.Fatalf("readme.txt didn't contain expected string - found '%s'\n", readmeData)
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
		t.Fatalf("sean.txt didn't contain expected string. expected '%s', found '%s'\n", expectedData, seanData)
	}

	// Test that there are the expected children in a directory
	foo, ok := root.ChildByName("foo")
	if !ok {
		t.Fatal("couldn't open foo")
	}
	fooDir := foo.(*Directory)
	numFooChildren := fooDir.NumChildren()
	if numFooChildren != 2 {
		t.Fatalf("expected 2 children, found %v\n", numFooChildren)
	}
	_, ok = fooDir.ChildByName("gopher.txt")
	if !ok {
		t.Fatal("couldn't get gopher.txt")
	}
	_, ok = fooDir.ChildByName("todo2.txt")
	if !ok {
		t.Fatal("couldn't get todo2.txt")
	}
	//t.Fail()
}
