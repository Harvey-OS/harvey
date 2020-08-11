package tmpfs

import (
	"archive/tar"
	"bytes"
	"log"
	"os"
	"strings"
	"testing"
)

// Create and add some files to the archive.
func createTestImageTar() *bytes.Buffer {
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

func TestReadArchiveTar(t *testing.T) {
	arch, err := ReadImageTar(createTestImageTar())
	if err != nil {
		t.Fatal(err)
	}

	// Read root
	root := arch.Root()
	if root.Name() != "/" {
		t.Fatal("incorrect root name")
	}

	// This really should be redone with a table.
	// Read readme.txt
	readme, err := root.ChildByName("readme.txt")
	if err != nil {
		t.Fatal(err)
	}
	readmeFile := readme.(*File)
	readmeData := string(readmeFile.Data())
	if readmeData != "This archive contains some text files." {
		t.Fatalf("readme.txt didn't contain expected string - found '%s'\n", readmeData)
	}

	// Read abc/123/sean.txt
	abc, err := root.ChildByName("abc")
	if err != nil {
		t.Fatal(err)
	}
	abcDir := abc.(*Directory)
	oneTwoThree, err := abcDir.ChildByName("123")
	if err != nil {
		t.Fatal(err)
	}
	oneTwoThreeDir := oneTwoThree.(*Directory)
	sean, err := oneTwoThreeDir.ChildByName("sean.txt")
	if err != nil {
		t.Fatal(err)
	}
	seanFile := sean.(*File)
	seanData := string(seanFile.Data())
	expectedData := "lorem ipshum."
	if seanData != expectedData {
		t.Fatalf("sean.txt didn't contain expected string. expected '%s', found '%s'\n", expectedData, seanData)
	}

	// Test that there are the expected children in a directory
	foo, err := root.ChildByName("foo")
	if err != nil {
		t.Fatal(err)
	}
	fooDir := foo.(*Directory)
	numFooChildren := fooDir.NumChildren()
	if numFooChildren != 2 {
		t.Fatalf("expected 2 children, found %v\n", numFooChildren)
	}
	_, err = fooDir.ChildByName("gopher.txt")
	if err != nil {
		t.Fatal(err)
	}
	_, err = fooDir.ChildByName("todo2.txt")
	if err != nil {
		t.Fatal(err)
	}
}

func TestReadArchiveCpio(t *testing.T) {
	Debug = t.Logf
	f, err := os.Open("test.cpio")
	if err != nil {
		t.Fatal(err)
	}
	defer f.Close()
	arch, err := ReadImageCpio(f)
	if err != nil {
		t.Fatal(err)
	}

	// Read root
	root := arch.Root()
	if root.Name() != "/" {
		t.Fatal("incorrect root name")
	}

	// Read bbin/bb
	bbin, err := root.ChildByName("bbin")
	if err != nil {
		t.Fatal(err)
	}
	bd, ok := bbin.(*Directory)
	if !ok {
		t.Fatal("cast bbin to directory: got ! ok, want ok")
	}
	bb, err := bd.ChildByName("bb")
	if err != nil {
		t.Fatal(err)
	}
	bbd := string(bb.(*File).Data())
	if bbd != "hi\n" {
		t.Fatalf("reading /bbin/bb: got %q, want %q", bbd, "hi\n")
	}

	for _, n := range []string{"cat", "date"} {

		i, err := bd.ChildByName(n)
		if err != nil {
			t.Fatal(err)
		}
		d := string(i.(*File).Data())
		if d != bbd {
			t.Fatalf("Reading %q: got %q, want %q", n, d, bbd)
		}
	}
}
