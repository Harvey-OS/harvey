// Copyright 2012 The Ninep Authors.  All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.
// This code is imported from the old ninep repo,
// with some changes.

package protocol

import (
	"bytes"
	"fmt"
	"io"
	"net"
	"sync"
	"time"
)

const DefaultAddr = ":5640"

type NsCreator func() NineServer

type Listener struct {
	nsCreator NsCreator

	// TCP address to listen on, default is DefaultAddr
	Addr string

	// Trace function for logging
	Trace Tracer

	// mu guards below
	mu sync.Mutex

	listeners map[net.Listener]struct{}
}

// Server is a 9p server.
// For now it's extremely serial. But we will use a chan for replies to ensure that
// we can go to a more concurrent one later.
type Server struct {
	NS NineServer
	D  Dispatcher

	// Versioned is set to true on the first call to Tversion
	Versioned bool
}

type conn struct {
	listener *Listener

	// server on which the connection arrived.
	server *Server

	// rwc is the underlying network connection.
	rwc net.Conn

	// remoteAddr is rwc.RemoteAddr().String(). See note in net/http/server.go.
	remoteAddr string

	// replies
	replies chan RPCReply

	// dead is set to true when we finish reading packets.
	dead bool
}

func NewListener(nsCreator NsCreator, opts ...ListenerOpt) (*Listener, error) {
	l := &Listener{
		nsCreator: nsCreator,
	}

	for _, o := range opts {
		if err := o(l); err != nil {
			return nil, err
		}
	}

	return l, nil
}

func (l *Listener) newConn(rwc net.Conn) (*conn, error) {
	ns := l.nsCreator()
	server := &Server{NS: ns, D: Dispatch}

	c := &conn{
		server:   server,
		listener: l,
		rwc:      rwc,
		replies:  make(chan RPCReply, NumTags),
	}

	return c, nil
}

// trackListener from http.Server
func (l *Listener) trackListener(ln net.Listener, add bool) {
	l.mu.Lock()
	defer l.mu.Unlock()

	if l.listeners == nil {
		l.listeners = make(map[net.Listener]struct{})
	}

	if add {
		l.listeners[ln] = struct{}{}
	} else {
		delete(l.listeners, ln)
	}
}

// closeListenersLocked from http.Server
func (l *Listener) closeListenersLocked() error {
	var err error
	for ln := range l.listeners {
		if cerr := ln.Close(); cerr != nil && err == nil {
			err = cerr
		}
		delete(l.listeners, ln)
	}
	return err
}

// Serve accepts incoming connections on the Listener and calls e.Accept on
// each connection.
func (l *Listener) Serve(ln net.Listener) error {
	defer ln.Close()

	var tempDelay time.Duration // how long to sleep on accept failure

	l.trackListener(ln, true)
	defer l.trackListener(ln, false)

	// from http.Server.Serve
	for {
		conn, err := ln.Accept()
		if err != nil {
			if ne, ok := err.(net.Error); ok && ne.Temporary() {
				if tempDelay == 0 {
					tempDelay = 5 * time.Millisecond
				} else {
					tempDelay *= 2
				}
				if max := 1 * time.Second; tempDelay > max {
					tempDelay = max
				}
				l.logf("ufs: Accept error: %v; retrying in %v", err, tempDelay)
				time.Sleep(tempDelay)
				continue
			}
			return err
		}
		tempDelay = 0

		if err := l.Accept(conn); err != nil {
			return err
		}
	}
}

// Accept a new connection, typically called via Serve but may be called
// directly if there's a connection from an exotic listener.
func (l *Listener) Accept(conn net.Conn) error {
	c, err := l.newConn(conn)
	if err != nil {
		return err
	}

	go c.serve()
	return nil
}

// Shutdown closes all active listeners. It does not close all active
// connections but probably should.
func (l *Listener) Shutdown() error {
	l.mu.Lock()
	defer l.mu.Unlock()

	return l.closeListenersLocked()
}

func (l *Listener) String() string {
	// TODO
	return ""
}

func (l *Listener) logf(format string, args ...interface{}) {
	if l.Trace != nil {
		l.Trace(format, args...)
	}
}

func (c *conn) String() string {
	return fmt.Sprintf("Dead %v %d replies pending", c.dead, len(c.replies))
}

func (c *conn) logf(format string, args ...interface{}) {
	// prepend some info about the conn
	c.listener.logf("[%v] "+format, append([]interface{}{c.remoteAddr}, args...)...)
}

func (c *conn) serve() {
	if c.rwc == nil {
		c.dead = true
		return
	}

	c.remoteAddr = c.rwc.RemoteAddr().String()

	defer c.rwc.Close()

	c.logf("Starting readNetPackets")

	for !c.dead {
		l := make([]byte, 7)
		if n, err := c.rwc.Read(l); err != nil || n < 7 {
			c.logf("readNetPackets: short read: %v", err)
			c.dead = true
			return
		}
		sz := int64(l[0]) + int64(l[1])<<8 + int64(l[2])<<16 + int64(l[3])<<24
		t := MType(l[4])
		b := bytes.NewBuffer(l[5:])
		r := io.LimitReader(c.rwc, sz-7)
		if _, err := io.Copy(b, r); err != nil {
			c.logf("readNetPackets: short read: %v", err)
			c.dead = true
			return
		}
		c.logf("readNetPackets: got %v, len %d, sending to IO", RPCNames[MType(l[4])], b.Len())
		//panic(fmt.Sprintf("packet is %v", b.Bytes()[:]))
		//panic(fmt.Sprintf("s is %v", s))
		if err := c.server.D(c.server, b, t); err != nil {
			c.logf("%v: %v", RPCNames[MType(l[4])], err)
		}
		c.logf("readNetPackets: Write %v back", b)
		amt, err := c.rwc.Write(b.Bytes())
		if err != nil {
			c.logf("readNetPackets: write error: %v", err)
			c.dead = true
			return
		}
		c.logf("Returned %v amt %v", b, amt)
	}
}

// Dispatch dispatches request to different functions.
// It's also the the first place we try to establish server semantics.
// We could do this with interface assertions and such a la rsc/fuse
// but most people I talked do disliked that. So we don't. If you want
// to make things optional, just define the ones you want to implement in this case.
func Dispatch(s *Server, b *bytes.Buffer, t MType) error {
	switch t {
	case Tversion:
		s.Versioned = true
	default:
		if !s.Versioned {
			m := fmt.Sprintf("Dispatch: %v not allowed before Tversion", RPCNames[t])
			// Yuck. Provide helper.
			d := b.Bytes()
			MarshalRerrorPkt(b, Tag(d[0])|Tag(d[1])<<8, m)
			return fmt.Errorf("Dispatch: %v not allowed before Tversion", RPCNames[t])
		}
	}

	switch t {
	case Tversion:
		return s.SrvRversion(b)
	case Tattach:
		return s.SrvRattach(b)
	case Tflush:
		return s.SrvRflush(b)
	case Twalk:
		return s.SrvRwalk(b)
	case Topen:
		return s.SrvRopen(b)
	case Tcreate:
		return s.SrvRcreate(b)
	case Tclunk:
		return s.SrvRclunk(b)
	case Tstat:
		return s.SrvRstat(b)
	case Twstat:
		return s.SrvRwstat(b)
	case Tremove:
		return s.SrvRremove(b)
	case Tread:
		return s.SrvRread(b)
	case Twrite:
		return s.SrvRwrite(b)
	}

	// This has been tested by removing Attach from the switch.
	ServerError(b, fmt.Sprintf("Dispatch: %v not supported", RPCNames[t]))
	return nil
}
