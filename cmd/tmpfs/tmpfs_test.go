package main

import (
	"archive/tar"
	"bytes"
	"compress/gzip"
	"fmt"
	"log"
	"net"
	"testing"

	"github.com/Harvey-OS/ninep/protocol"
	"harvey-os.org/go/internal/tmpfs"
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

func TestNwTmpfs(t *testing.T) {
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

func TestMount(t *testing.T) {
	p, p2 := net.Pipe()

	c, err := protocol.NewClient(func(c *protocol.Client) error {
		c.FromNet, c.ToNet = p, p
		return nil
	},
		func(c *protocol.Client) error {
			c.Msize = 8192
			c.Trace = print //t.Logf
			return nil
		})
	if err != nil {
		t.Fatalf("%v", err)
	}
	t.Logf("Client is %v", c.String())

	arch := tmpfs.ReadImage(createTestImage())
	n, err := newTmpfs(arch, func(l *protocol.Listener) error {
		l.Trace = print //t.Logf
		return nil
	})
	if err != nil {
		t.Fatal(err)
	}

	if err := n.Accept(p2); err != nil {
		t.Fatalf("Accept: want nil, got %v", err)
	}

	t.Logf("n is %v", n)

	m, v, err := c.CallTversion(8000, "9P2000")
	if err != nil {
		t.Fatalf("CallTversion: want nil, got %v", err)
	}
	t.Logf("CallTversion: msize %v version %v", m, v)

	t.Logf("Server is %v", n.String())

	a, err := c.CallTattach(0, protocol.NOFID, "/", "")
	if err != nil {
		t.Fatalf("CallTattach: want nil, got %v", err)
	}

	t.Logf("Attach is %v", a)
	w, err := c.CallTwalk(0, 1, []string{"hi", "there"})
	if err == nil {
		t.Fatalf("CallTwalk(0,1,[\"hi\", \"there\"]): want err, got nil")
	}
	if len(w) > 0 {
		t.Fatalf("CallTwalk(0,1,[\"hi\", \"there\"]): want 0 QIDs, got %v", w)
	}
	t.Logf("Walk is %v", w)

	reqPath := []string{"readme.txt"}
	w, err = c.CallTwalk(0, 1, reqPath)
	if err != nil {
		t.Fatalf("CallTwalk(0,1,%v): want nil, got %v", reqPath, err)
	}
	t.Logf("Walk is %v", w)

	// Read readme.txt
	b, err := c.CallTread(1, 0, 8192)
	if err != nil {
		t.Fatalf("CallTread(1, 0, 8192): want nil, got %v", err)
	}
	expectedData := "This archive contains some text files."
	if string(b[:]) != expectedData {
		t.Fatalf("CallTread(1, 0, 8192): expected '%s', found '%s'", expectedData, string(b))
	}

	reqPath = []string{"foo", "gopher.txt"}
	w, err = c.CallTwalk(0, 2, reqPath)
	if err != nil {
		t.Fatalf("CallTwalk(0,2,%v): want nil, got %v", reqPath, err)
	}
	t.Logf("Walk is %v", w)

	// readdir test
	w, err = c.CallTwalk(0, 3, []string{"foo"})
	if err != nil {
		t.Fatalf("CallTwalk(0, 3, []string{\"foo\"}): want nil, got %v", err)
	}
	t.Logf("Walk is %v", w)
	_, _, err = c.CallTopen(3, protocol.OWRITE)
	if err == nil {
		t.Fatalf("CallTopen(3, protocol.OWRITE) on /: want err, got nil")
	}
	_, _, err = c.CallTopen(3, protocol.ORDWR)
	if err == nil {
		t.Fatalf("CallTopen(3, protocol.ORDWR) on /: want err, got nil")
	}
	_, _, err = c.CallTopen(3, protocol.OREAD)
	if err != nil {
		t.Fatalf("CallTopen(3, protocol.OREAD): want nil, got %v", err)
	}

	// Read folder 'foo'
	var o protocol.Offset
	var iter int
	for iter < 10 {
		iter++
		b, err := c.CallTread(3, o, 256)
		// EOF is indicated by a zero length read, not an actual error
		if len(b) == 0 {
			break
		}
		if err != nil {
			t.Fatalf("CallTread(3, 0, 256): want nil, got %v", err)
		}

		dent, err := protocol.Unmarshaldir(bytes.NewBuffer(b))
		if err != nil {
			t.Errorf("Unmarshalldir: want nil, got %v", err)
		}
		t.Logf("dir read is %v", dent)
		o += protocol.Offset(len(b))
	}

	if iter > 4 {
		t.Errorf("Too many reads from the directory: want 3, got %v", iter)
	}
	if err := c.CallTclunk(3); err != nil {
		t.Fatalf("CallTclunk(3): want nil, got %v", err)
	}

	// Read cloned root folder
	w, err = c.CallTwalk(0, 4, []string{})
	if err != nil {
		t.Fatalf("CallTwalk(0,4,%v): want nil, got %v", reqPath, err)
	}

	_, err = c.CallTread(4, 0, 256)
	if err != nil {
		t.Fatalf("CallTread(0, 0, 256): want nil, got %v", err)
	}

	// readdir test (bar)
	w, err = c.CallTwalk(0, 5, []string{"bar"})
	if err != nil {
		t.Fatalf("CallTwalk(0, 3, []string{\"bar\"}): want nil, got %v", err)
	}
	barRawDir, err := c.CallTread(5, 0, 256)
	if err != nil {
		t.Fatalf("CallTread(5, 0, 256): want nil, got %v", err)
	}
	barDir, err := protocol.Unmarshaldir(bytes.NewBuffer(barRawDir))
	if err != nil {
		t.Errorf("Unmarshalldir: want nil, got %v", err)
	}
	t.Logf("dir read is %v", barDir)

	//t.Fail()
}
