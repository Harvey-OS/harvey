package main

import (
	"fmt"
	"io"
	"log"
	"net"
	"os"
	"os/signal"
	"strings"
	"syscall"
	"unsafe"
)

const NCCS = 32

type (
	cc_t     byte
	speed_t  uint32
	tcflag_t uint32

	termios struct {
		c_iflag, c_oflag, c_cflag, c_lflag tcflag_t
		c_line                             cc_t
		c_cc                               [NCCS]cc_t
		c_ispeed, c_ospeed                 speed_t
	}

	winsize struct {
		ws_row, ws_col       uint16
		ws_xpixel, ws_ypixel uint16
	}
)

// termios constants
const (
	t_IGNBRK = tcflag_t(0000001)
	t_BRKINT = tcflag_t(0000002)
	t_PARMRK = tcflag_t(0000010)
	t_ISTRIP = tcflag_t(0000040)
	t_INLCR  = tcflag_t(0000100)
	t_IGNCR  = tcflag_t(0000200)
	t_ICRNL  = tcflag_t(0000400)
	t_IXON   = tcflag_t(0002000)
	t_OPOST  = tcflag_t(0000001)
	t_ECHO   = tcflag_t(0000010)
	t_ECHONL = tcflag_t(0000100)
	t_ICANON = tcflag_t(0000002)
	t_ISIG   = tcflag_t(0000001)
	t_IEXTEN = tcflag_t(0100000)
	t_CSIZE  = tcflag_t(0000060)
	t_CS8    = tcflag_t(0000060)
	t_PARENB = tcflag_t(0000400)
	t_VTIME  = 5
	t_VMIN   = 6
)

// ioctl constants
const (
	i_TCGETS     = 0x5401
	i_TCSETS     = 0x5402
	i_TIOCGWINSZ = 0x5413
	i_TIOCSWINSZ = 0x5414
)

var unrawterm termios

func getTermios(fd uintptr) (*termios, error) {
	term := new(termios)
	if err := term.get(fd); err != nil {
		return nil, err
	}

	return term, nil
}

func (self *termios) get(fd uintptr) error {
	r1, _, errno := syscall.Syscall(syscall.SYS_IOCTL,
		fd, uintptr(i_TCGETS),
		uintptr(unsafe.Pointer(self)))

	if errno != 0 || r1 != 0 {
		return fmt.Errorf("termios.get: r1 %v, errno %v", r1, errno)
	}
	return nil
}

func (self *termios) set(fd uintptr) error {
	r1, _, errno := syscall.Syscall(syscall.SYS_IOCTL,
		fd, uintptr(i_TCSETS),
		uintptr(unsafe.Pointer(self)))
	if errno != 0 || r1 != 0 {
		return fmt.Errorf("termios.get: r1 %v, errno %v", r1, errno)
	}

	return nil
}

func (self *termios) setRaw(fd uintptr) error {
	self.c_iflag &= ^(t_IGNBRK | t_BRKINT | t_PARMRK | t_ISTRIP |
		t_INLCR | t_IGNCR | t_ICRNL | t_IXON)
	self.c_oflag &= ^t_OPOST
	self.c_lflag &= ^(t_ECHO | t_ECHONL | t_ICANON | t_ISIG | t_IEXTEN)
	self.c_cflag &= ^(t_CSIZE | t_PARENB)
	self.c_cflag |= t_CS8

	self.c_cc[t_VMIN] = 1
	self.c_cc[t_VTIME] = 0

	return self.set(fd)
}

func getWinsize(fd uintptr) (*winsize, error) {
	size := new(winsize)
	if err := size.get(fd); err != nil {
		return nil, err
	}

	return size, nil
}

func (self *winsize) get(fd uintptr) error {
	r1, _, errno := syscall.Syscall(syscall.SYS_IOCTL,
		fd, uintptr(i_TIOCGWINSZ),
		uintptr(unsafe.Pointer(self)))
	if errno != 0 || r1 != 0 {
		return fmt.Errorf("termios.get: r1 %v, errno %v", r1, errno)
	}

	return nil
}

func (self *winsize) set(fd uintptr) error {
	r1, _, errno := syscall.Syscall(syscall.SYS_IOCTL,
		fd, uintptr(i_TIOCSWINSZ),
		uintptr(unsafe.Pointer(self)))
	if errno != 0 || r1 != 0 {
		return fmt.Errorf("termios.get: r1 %v, errno %v", r1, errno)
	}

	return nil
}

func raw() {
	// we don't set raw until the very last, so if they see an issue they can hit ^C
	t, err := getTermios(1)
	if err != nil {
		log.Fatalf(err.Error())
	}
	unrawterm = *t
	if err = t.setRaw(1); err != nil {
		log.Fatalf(err.Error())
	}
}

func unraw() {
	unrawterm.set(1)
}

// Copyright 2012 the u-root Authors. All rights reserved
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

/*
Wget reads one file from the argument and writes it on the standard output.
*/
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

	raw()
	defer unraw()

	sigc := make(chan os.Signal, 1)
	signal.Notify(sigc, os.Interrupt, os.Kill, syscall.SIGTERM)
	go func() {
		for _ = range sigc {
			unraw()
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
				fmt.Printf("%s", string(b[i]))
				if b[i] == '\r' {
					b[i] = '\n'
					fmt.Printf("\n")
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
	outchan := make(chan string)
	for i, c := range tcpc {
		go func(i int, c *net.TCPConn) {

			b := make([]byte, 256)
			var out []byte
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
				for _, v := range b[:n] {
					out = append(out, v)
					if v == '\n' {
						outchan <- fmt.Sprintf("[%d]:%s\r", i, out)
						out = nil
					}
				}
			}
		}(i, c)
	}
	go func() {
		for {
			line := <-outchan
			fmt.Printf("%s", line)
		}
	}()

	for _ = range tcpc {
		<-xchan
	}
}
