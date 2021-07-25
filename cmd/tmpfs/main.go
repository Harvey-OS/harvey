// Binary p9ufs provides a local 9P2000.L server for the p9 package.
//
// To use, first start the server:
//     htmpfs xxx.tgz
//
// Then, connect using the Linux 9P filesystem:
//     mount -t 9p -o trans=tcp,port=5641 127.0.0.1 /mnt

package main

import (
	"archive/tar"
	"bytes"
	"compress/gzip"
	"flag"
	"fmt"
	"io"
	"log"
	"net"
	"strings"

	"github.com/hugelgupf/p9/fsimpl/templatefs"
	"github.com/hugelgupf/p9/p9"
	"github.com/u-root/u-root/pkg/ulog"
)

var (
	networktype = flag.String("ntype", "tcp4", "Default network type")
	netaddr     = flag.String("addr", ":5641", "Network address")
	verbose     = flag.Bool("verbose", false, "Verbose")
)

type entry interface {
}

type file struct {
	templatefs.NotDirectoryFile
	templatefs.ReadOnlyFile

	mode int64         // Permissions
	data *bytes.Buffer // File contents
}

type directory struct {
	templatefs.IsDir
	templatefs.ReadOnlyDir

	entries map[string]entry
}

type attacher struct {
	root entry
}

func newDirectory() *directory {
	return &directory{entries: map[string]entry{}}
}

func (d *directory) addFile(filepath string, file *file) error {
	filecmps := strings.Split(filepath, "/")
	if dir, err := d.getOrCreateDir(filecmps[:len(filecmps)-1]); err != nil {
		return err
	} else {
		return dir.createFile(filecmps[len(filecmps)-1], file)
	}
}

func (d *directory) getOrCreateDir(cmps []string) (*directory, error) {
	if len(cmps) == 0 {
		return d, nil
	}

	cmpname := cmps[0]
	if entry, exists := d.entries[cmpname]; exists {
		if dir, ok := entry.(directory); ok {
			return dir.getOrCreateDir(cmps[1:])
		} else {
			return nil, fmt.Errorf("File already exists with name %s", cmpname)
		}
	} else {
		newDir := newDirectory()
		d.entries[cmpname] = newDir
		return newDir.getOrCreateDir(cmps[1:])
	}
}

func (d *directory) createFile(filename string, file *file) error {
	if _, exists := d.entries[filename]; exists {
		return fmt.Errorf("File or directory already exists with name %s", filename)
	}
	d.entries[filename] = file
	return nil
}

func (d *directory) qid() (p9.QID, error) {
	var qid p9.QID
	qid.Type = p9.TypeDir
	qid.Path = 
	return qid, nil
}

func newAttacher(e entry) *attacher {
	return &attacher{root: e}
}

func (a *attacher) Attach() (p9.File, error) {
	return a.root, nil
}

func main() {
	flag.Parse()

	// Create and add some files to the archive.
	buf := createTestImage()
	files := readImage(buf)

	// Bind and listen on the socket.
	listener, err := net.Listen(*networktype, *netaddr)
	if err != nil {
		log.Fatalf("err binding: %v", err)
	}

	var opts []p9.ServerOpt
	if *verbose {
		opts = append(opts, p9.WithServerLogger(ulog.Log))
	}

	// Run the server.
	server := p9.NewServer(newAttacher(files), opts...)
	server.Serve(listener)
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

// Read a compressed tar and produce a file hierarchy
func readImage(buf *bytes.Buffer) entry {
	gzr, err := gzip.NewReader(buf)
	if err != nil {
		log.Fatal(err)
	}
	defer func() {
		if err := gzr.Close(); err != nil {
			log.Fatal(err)
		}
	}()

	dir := &directory{}
	tr := tar.NewReader(gzr)
	for {
		hdr, err := tr.Next()
		if err == io.EOF {
			break // End of archive
		}
		if err != nil {
			log.Fatal(err)
		}

		filename := hdr.Name
		file := &file{
			mode: hdr.Mode,
			data: &bytes.Buffer{},
		}
		if _, err := io.Copy(file.data, tr); err != nil {
			log.Fatal(err)
		}

		dir.addFile(filename, file)
		//files[filename] = file
	}

	return dir
}
