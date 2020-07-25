// UFS is a userspace server which exports a filesystem over 9p2000.
//
// By default, it will export / over a TCP on port 5640 under the username
// of "harvey".
package main

import (
	"flag"
	"log"
	"net"

	ufs "github.com/Harvey-OS/ninep/filesystem"
	"github.com/Harvey-OS/ninep/protocol"
)

var (
	ntype = flag.String("ntype", "tcp4", "Default network type")
	naddr = flag.String("addr", ":5640", "Network address")
)

func main() {
	flag.Parse()

	ln, err := net.Listen(*ntype, *naddr)
	if err != nil {
		log.Fatalf("Listen failed: %v", err)
	}

	ufslistener, err := ufs.NewUFS(func(l *protocol.Listener) error {
		l.Trace = nil // log.Printf
		return nil
	})

	if err := ufslistener.Serve(ln); err != nil {
		log.Fatal(err)
	}
}
