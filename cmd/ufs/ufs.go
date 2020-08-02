// UFS is a userspace server which exports a filesystem over 9p2000.
//
// By default, it will export / over a TCP on port 5640 under the username
// of "harvey".
package main

import (
	"flag"
	"log"
	"net"

	"harvey-os.org/ninep/protocol"
	"harvey-os.org/ninep/ufs"
)

var (
	ntype = flag.String("net", "tcp4", "Default network type")
	naddr = flag.String("addr", ":5640", "Network address")
	debug = flag.Int("debug", 0, "print debug messages")
	root  = flag.String("root", "/", "Set the root for all attaches")
)

func main() {
	flag.Parse()

	ln, err := net.Listen(*ntype, *naddr)
	if err != nil {
		log.Fatalf("Listen failed: %v", err)
	}

	ufslistener, err := ufs.NewUFS(*root, *debug, func(l *protocol.NetListener) error {
		l.Trace = nil
		if *debug > 1 {
			l.Trace = log.Printf
		}
		return nil
	})

	if err := ufslistener.Serve(ln); err != nil {
		log.Fatal(err)
	}
}
