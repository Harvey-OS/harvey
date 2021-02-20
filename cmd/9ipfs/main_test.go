package main

import (
	"bytes"
	"context"
	"io"
	"io/ioutil"
	"log"
	"net"
	"os"
	"testing"

	"harvey-os.org/cmd/9ipfs/internal/ipfs"
	"harvey-os.org/ninep"
	"harvey-os.org/ninep/protocol"
)

func TestFileServer(t *testing.T) {
	ctx, cancel := context.WithCancel(context.Background())
	defer cancel()

	repoDir, err := ioutil.TempDir("", "9ipfs")
	if err != nil {
		t.Fatal(err)
	}
	defer os.RemoveAll(repoDir)

	ipfsNode, err := ipfs.New(ctx, &ipfs.Config{
		LogWriter:   os.Stderr,
		TempRepo:    true,
		TempRepoDir: repoDir,
	})
	if err != nil {
		t.Fatal(err)
	}
	testdata, err := ipfsNode.Put(ctx, "testdata")
	if err != nil {
		t.Fatal(err)
	}

	nsCreator := func() protocol.NineServer {
		fs := newFileServer(ipfsNode)
		if testing.Verbose() {
			return &ninep.DebugFileServer{FileServer: fs}
		}
		return fs
	}
	listener, err := protocol.NewNetListener(nsCreator, func(l *protocol.NetListener) error {
		//l.Trace = t.Logf
		return nil
	})
	if err != nil {
		t.Fatal(err)
	}

	p1, p2 := net.Pipe()
	c, err := protocol.NewClient(
		func(c *protocol.Client) error {
			c.FromNet, c.ToNet = p1, p1
			return nil
		},
		func(c *protocol.Client) error {
			c.Msize = 8192
			//c.Trace = t.Logf
			c.Trace = log.New(io.Discard, "", log.LstdFlags).Printf
			return nil
		},
	)
	if err != nil {
		t.Fatal(err)
	}

	if err := listener.Accept(p2); err != nil {
		t.Fatal(err)
	}

	_, _, err = c.CallTversion(8000, "9P2000")
	if err != nil {
		t.Fatalf("CallTversion: want nil, got %v", err)
	}

	_, err = c.CallTattach(0, protocol.NOFID, "/", "")
	if err != nil {
		t.Fatalf("CallTattach: want nil, got %v", err)
	}
	defer clunk(t, c, 0)

	_, err = c.CallTwalk(0, 1, []string{testdata.Cid.String()})
	if err != nil {
		t.Fatalf("CallTwalk(0,1,...): want nil, got %v", err)
	}
	defer clunk(t, c, 1)

	_, err = c.CallTwalk(1, 2, []string{"hello.txt"})
	if err != nil {
		t.Fatalf("CallTwalk(1,2,...): want nil, got %v", err)
	}
	defer clunk(t, c, 2)

	dirs, err := readDir(c, 1)
	if err != nil {
		t.Fatal(err)
	}
	if len(dirs) != 1 || dirs[0].Name != "hello.txt" {
		t.Fatalf("directory entries: %v", dirs)
	}

	_, _, err = c.CallTopen(2, protocol.OREAD)
	if err != nil {
		t.Fatalf("CallTopen(2, protocol.OREAD): want nil, got %v", nil)
	}

	b, err := c.CallTread(2, 0, 128)
	if err != nil {
		t.Fatalf("CallTread(2, ...): want nil, got %v", err)
	}

	if got, want := string(b), "Hello, IPFS!\n"; got != want {
		t.Errorf("read returned %q, want %q", got, want)
	}
}

func clunk(t *testing.T, c *protocol.Client, fid protocol.FID) {
	t.Helper()

	if err := c.CallTclunk(fid); err != nil {
		t.Errorf("CallTclunk(%v) failed: %v", fid, err)
	}
}

func readDir(c *protocol.Client, fid int) ([]protocol.Dir, error) {
	const STATMAX = 65535
	var out []protocol.Dir
	off := 0
	for {
		b, err := c.CallTread(protocol.FID(fid), protocol.Offset(off), STATMAX)
		if err != nil {
			return nil, err
		}
		if len(b) == 0 {
			break
		}
		off += len(b)

		buf := bytes.NewBuffer(b)
		for buf.Len() > 0 {
			d, err := protocol.Unmarshaldir(buf)
			if err != nil {
				return nil, err
			}
			out = append(out, d)
		}

	}
	return out, nil
}
