package main

import (
	"bytes"
	"fmt"
	"golang.org/x/crypto/ssh/terminal"
	"io"
	"net"
	"os"
	"os/signal"
	"strings"
	"syscall"
)

// Copyright 2012 the u-root Authors. All rights reserved
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

type (
	chunk struct {
		id  int
		buf []byte
	}
)

const (
	echo = false
)

func comprefix(lines [][]byte) (int, int) {
	var line0 []byte
	len0 := 0
	for _, ln := range lines {
		if len(ln) > len0 {
			line0 = ln
			len0 = len(ln)
		}
	}
	ndiff := 0
	for i := range line0 {
		nshort := 0
		for _, ln := range lines {
			if i >= len(ln) {
				nshort++
			} else if ln[i] != line0[i] {
				ndiff++
			}
		}
		if ndiff > 0 {
			return i, ndiff
		}
		if nshort > 0 {
			return i, ndiff
		}
	}
	return len(line0), ndiff
}

func main() {

	if len(os.Args) < 2 {
		os.Exit(1)
	}

	var conns []*net.TCPConn
	for _, a := range os.Args[1:] {
		port := "1522"
		if !strings.Contains(a, ":") {
			a = a + ":" + port
		}

		tcpdst, err := net.ResolveTCPAddr("tcp", a)
		if err != nil {
			fmt.Printf("%v\n", err)
			os.Exit(1)
		}

		c, err := net.DialTCP("tcp", nil, tcpdst)
		if err != nil {
			fmt.Printf("%v\n", err)
			os.Exit(1)
		}
		conns = append(conns, c)
	}

	unraw, err := terminal.MakeRaw(0)
	if err != nil {
		fmt.Printf("%v\n", err)
		os.Exit(1)
	}
	defer terminal.Restore(0, unraw)

	sigc := make(chan os.Signal, 1)
	signal.Notify(sigc, os.Interrupt, os.Kill, syscall.SIGTERM)
	go func() {
		for _ = range sigc {
			terminal.Restore(0, unraw)
			os.Exit(1)
		}
	}()

	go func() {
		buf := make([]byte, 256)
		for {
			n, err := os.Stdin.Read(buf)
			if err != nil {
				if err != io.EOF {
					fmt.Printf("%v\n", err)
				}
				for _, c := range conns {
					c.CloseWrite() // I know, I know.. but it is handy sometimes.
				}
				break
			}
			buf = buf[0:n]
			for i := range buf {
				if echo {
					fmt.Printf("%s", string(buf[i]))
				}
				if buf[i] == '\r' {
					buf[i] = '\n'
					if echo {
						fmt.Printf("\n")
					}
				}
			}

			for _, c := range conns {
				if _, err := c.Write(buf); err != nil {
					fmt.Printf("%v\n", err)
					break
				}
			}

		}
	}()

	waitc := make(chan int)
	chunkc := make(chan chunk)
	for id, c := range conns {
		go func(id int, c *net.TCPConn) {
			for {
				buf := make([]byte, 256)
				n, err := c.Read(buf)
				if err != nil {
					if err != io.EOF {
						fmt.Printf("%v\n", err)
					}
					c.Close()
					waitc <- 1
					break
				}
				if n == 0 {
					continue
				}
				chunkc <- chunk{id, buf[0:n]}
			}
		}(id, c)
	}

	act := make([][]byte, len(conns))

	go func() {
		oldpfix := 0
		insync := true
		for {
			chunk, more := <-chunkc
			if !more {
				break
			}
			id := chunk.id
			for _, b := range chunk.buf {
				act[id] = append(act[id], b)
				if b == '\b' {
					act[id] = append(act[id], ' ')
					act[id] = append(act[id], '\b')
				}
			}

			/* streams agree, so spit out all they agree on */
			if insync {
				newpfix, ndiff := comprefix(act)
				if ndiff == 0 && newpfix > oldpfix {
					os.Stdout.Write(act[id][oldpfix:newpfix])
					for {
						i := bytes.IndexByte(act[id][:newpfix], '\n')
						if i == -1 {
							break
						}
						for id := range act {
							clen := copy(act[id], act[id][i+1:])
							act[id] = act[id][0:clen]
						}
						newpfix = newpfix - (i + 1)
					}
					oldpfix = newpfix
				}

				if ndiff > 0 {
					insync = false
				}
			}

			/*
			 *	streams disagree, first to newline completes from oldpfix on.
			 *	the outer loop is just for transitioning from insync, where
			 *	a leader may be several lines ahead
			 */
			if !insync {
				for id := range act {
					for {
						i := bytes.IndexByte(act[id], '\n')
						if i != -1 {
							if oldpfix == 0 {
								fmt.Fprintf(os.Stdout, "[%d]", id)
							}
							os.Stdout.Write(act[id][oldpfix : i+1])
							clen := copy(act[id], act[id][i+1:])
							act[id] = act[id][0:clen]
							oldpfix = 0
						} else {
							break
						}
					}
				}

				/* try for prompt (or sheer luck) */
				if _, ndiff := comprefix(act); ndiff == 0 {
					insync = true
					oldpfix = 0
				}
			}
		}
		waitc <- 1
	}()

	for _ = range conns {
		<-waitc
	}
	close(chunkc)
	<-waitc
}
