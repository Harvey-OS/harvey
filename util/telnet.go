package main

import (
	"fmt"
	"io"
	"net"
	"os"
	"os/signal"
	"strings"
	"syscall"
	"golang.org/x/crypto/ssh/terminal"
)

// Copyright 2012 the u-root Authors. All rights reserved
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

/*
Wget reads one file from the argument and writes it on the standard output.
*/

const (
	echo = false
)

func main() {
	a := os.Args

	if len(a) < 2 {
		os.Exit(1)
	}

	var tcpc []*net.TCPConn
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
		tcpc = append(tcpc, c)
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
		b := make([]byte, 1024)
		for {
			n, err := os.Stdin.Read(b)
			if err != nil {
				if err != io.EOF {
					fmt.Printf("%v\n", err)
				}
				for _, c := range tcpc {
					c.CloseWrite() // I know, I know.. but it is handy sometimes.
				}
				break
			}
			for i := range b[:n] {
				if echo { fmt.Printf("%s", string(b[i])) }
				if b[i] == '\r' {
					b[i] = '\n'
					if echo { fmt.Printf("\n") }
				}
			}

			for _, c := range tcpc {
				if _, err := c.Write(b[:n]); err != nil {
					fmt.Printf("%v\n", err)
					break
				}
			}

		}
	}()

	xchan := make(chan int)
	outchan := make(chan []byte)
	for i, c := range tcpc {
		go func(i int, c *net.TCPConn) {

			b := make([]byte, 256)
			for {
				n, err := c.Read(b)
				if err != nil {
					if err != io.EOF {
						fmt.Printf("%v\n", err)
					}
					c.Close()
					xchan <- 1
					break
				}
				if n == 0 {
					continue
				}
				outchan <- append([]byte{}, b[:n]...)
			}
		}(i, c)
	}
	go func() {
		for {
			chunk := <-outchan
			os.Stdout.Write(chunk)
			//fmt.Printf("%s", line)
		}
	}()

	for _ = range tcpc {
		<-xchan
	}
}
