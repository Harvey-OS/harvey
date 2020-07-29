// +build plan9

package main

import (
	"bytes"
	"os"
	"testing"

	"harvey-os.org/internal/tmpfs"
	"harvey-os.org/pkg/ninep/protocol"
	"harvey-os.org/pkg/sys"
)

func TestNwTmpfs(t *testing.T) {
	arch, err := tmpfs.ReadImage(createTestImage())
	if err != nil {
		t.Fatal(err)
	}

	fd, err := sys.PostPipe(*srv)
	if err != nil {
		t.Fatal(err)
	}

	fs := &fileServer{archive: arch, files: make(map[protocol.FID]*FidEntry), ioUnit: 1 * 1024 * 1024}

	// TODO: get the tracing back in.
	// The ninep package was from a long time ago and it's
	// awkward at best.
	protocol.ServeFromRWC(os.NewFile(uintptr(fd), *srv), fs, *srv)
}

func TestMount(t *testing.T) {
	arch, err := tmpfs.ReadImage(createTestImage())
	if err != nil {
		t.Fatal(err)
	}

	fd, err := sys.PostPipe(*srv)
	if err != nil {
		t.Fatal(err)
	}

	fs := &fileServer{archive: arch, files: make(map[protocol.FID]*FidEntry), ioUnit: 1 * 1024 * 1024}

	// TODO: get the tracing back in.
	// The ninep package was from a long time ago and it's
	// awkward at best.
	protocol.ServeFromRWC(os.NewFile(uintptr(fd), *srv), fs, *srv)

	f, err := os.OpenFile(*srv, os.O_RDWR, 0)
	if err != nil {
		t.Fatal(err)
	}

	c, err := protocol.NewClient(func(c *protocol.Client) error {
		c.FromNet, c.ToNet = f, f
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
	m, v, err := c.CallTversion(8000, "9P2000")
	if err != nil {
		t.Fatalf("CallTversion: want nil, got %v", err)
	}
	t.Logf("CallTversion: msize %v version %v", m, v)

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
}
