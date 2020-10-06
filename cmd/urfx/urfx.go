// Copyright 2013-2020 the u-root Authors. All rights reserved
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// urfx operates on u-root cpio files.
// It is highly specialized to u-root for now.
// It converts symlinks to #!files of the form
// #!/bbin/bb #!name
// e.g.
// bbin/ls would be converted from a symlink to bb to
// #!/bbin/bb #!ls
// Note that no files with absolute paths are created;
// only references to absolute paths are. This is fine
// since on plan 9 any user can construct a namespace
// containing /bbin.
// In short, namespaces are why Plan 9 does not need
// PATH variables.
// urfx will convert all symlinks that resolve to bbin/bb.
// For example, should there be a bin/sh in the archive
// that links to ../bbin/rush, urfx will resolve that
// and create a file in bin/sh containing
// #!/bbin/bb #!rush
// To use this tool:
// urfx  < /tmp/initramfs.plan9_amd64.cpio > somefile.cpio
// or even
// urfx < /tmp/initramfs.plan9_amd64.cpio | (cd $HARVEY && cpio -i)
// once harvey boots:
// bind -b /bbin /bin
//
// Synopsis:
//     urfx [-v]
//
// Description:
//     urfx convert u-root cpio containing symlinks to a cpio containing #! files.
//
// Options:
//     -v: debug prints
package main

import (
	"flag"
	"fmt"
	"io"
	"io/ioutil"
	"log"
	"os"
	"path/filepath"

	"github.com/u-root/u-root/pkg/cpio"
	"github.com/u-root/u-root/pkg/uio"
)

const (
	format = "newc"
	// Not defined for Plan 9.
	S_IFLNK                                     = 0xa000
	S_IFMT                                      = 0xf000
)

type sl struct {
	cpio.Record
	root string
	orig string
}

var (
	debug = func(string, ...interface{}) {}
	d     = flag.Bool("v", false, "Debug prints")
)

func usage() {
	log.Fatalf("Usage: urfx")
}

func main() {
	flag.Parse()
	if *d {
		debug = log.Printf
	}

	a := flag.Args()
	debug("Args %v", a)
	if len(a) != 0 {
		usage()
	}

	archiver, err := cpio.Format(format)
	if err != nil {
		log.Fatalf("Format %q not supported: %v", format, err)
	}

	// The operation here is a passthrough. Only symlinks are
	// modified and only if their base is bb
	rr := archiver.Reader(os.Stdin)
	rw := archiver.Writer(os.Stdout)
	symlinks := map[string]*sl{}
	for {
		rec, err := rr.ReadRecord()
		if err == io.EOF {
			break
		}
		if err != nil {
			log.Fatalf("error reading records: %v", err)
		}
		debug("%v mode %#o %#o", rec.Name, rec.Mode, os.ModeType)

		if rec.Mode&S_IFMT != S_IFLNK {
			if err := rw.WriteRecord(rec); err != nil {
				log.Fatalf("Writing record %q failed: %v", rec.Name, err)
			}
			continue
		}

		orig, err := ioutil.ReadAll(uio.Reader(rec))
		if err != nil {
			log.Fatal(err)
		}

		// For Plan 9, adjust the symlink target so that it is relative to /. This makes it match file names.
		root, err := filepath.Rel("/", filepath.Join("/", filepath.Dir(rec.Name), string(orig)))
		if err != nil {
			log.Fatalf("Can't make %q relative to /!", filepath.Join("/", filepath.Dir(rec.Name)))
		}

		debug("Adding record for file %q: root %s orig %s\n", rec.Name, root, orig)
		symlinks[rec.Name] = &sl{Record: rec, root: string(root), orig: string(orig)}
	}

	for _, s := range symlinks {
		debug("Symlink map: name %q, root %q, orig %q", s.Name, s.root, s.orig)
		// we need to make all links shell scripts, and all
		// shell scripts terminal, i.e. the form of
		// #!/bin/bb #!command
		// so we are not starting intermediate
		// processes.
		cmd := fmt.Sprintf("#!/bin/bb #!%s\n", filepath.Base(s.Name))
		debug("Command is %s", cmd)
		if s.root != "bbin/bb" {
			debug("%s targets %s, follow link", s.Name, s.root)
			// Need to follow it to bin/bb. If it does not go there,
			// that's ok.
			t, ok := symlinks[s.root]
			for ok {
				debug("Check t.root %s", t.root)
				if t.root == "bbin/bb" {
					cmd = fmt.Sprintf("#!/bin/bb #!%s\n", filepath.Base(t.Name))
					debug("Set cmd to %s", cmd)
					break
				}
				t, ok = symlinks[t.root]
			}
			if !ok {
				debug("%s is not a bbin/bb target, skipping", s.Name)
				continue
			}
		}
		rec := cpio.StaticFile(s.Name, cmd, 0755)
		debug("rec is %v", rec)
		if err := rw.WriteRecord(rec); err != nil {
			log.Fatalf("Writing record %q failed: %v", rec.Name, err)
		}
	}

	if err := cpio.WriteTrailer(rw); err != nil {
		log.Fatalf("Error writing trailer record: %v", err)
	}

}
