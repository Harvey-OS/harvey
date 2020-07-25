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
	"log"
	"runtime"
	"sync/atomic"
)

// Client implements a 9p client. It has a chan containing all tags,
// a scalar FID which is incremented to provide new FIDS (all FIDS for a given
// client are unique), an array of MaxTag-2 RPC structs, a ReadWriteCloser
// for IO, and two channels for a server goroutine: one down which RPCalls are
// pushed and another from which RPCReplys return.
// Once a client is marked Dead all further requests to it will fail.
// The ToNet/FromNet are separate so we can use io.Pipe for testing.
type Client struct {
	Tags       chan Tag
	FID        uint64
	RPC        []*RPCCall
	ToNet      io.WriteCloser
	FromNet    io.ReadCloser
	FromClient chan *RPCCall
	FromServer chan *RPCReply
	Msize      uint32
	Dead       bool
	Trace      Tracer
}

func NewClient(opts ...ClientOpt) (*Client, error) {
	var c = &Client{}

	c.Tags = make(chan Tag, NumTags)
	for i := 1; i < int(NOTAG); i++ {
		c.Tags <- Tag(i)
	}
	c.FID = 1
	c.RPC = make([]*RPCCall, NumTags)
	for _, o := range opts {
		if err := o(c); err != nil {
			return nil, err
		}
	}
	c.FromClient = make(chan *RPCCall, NumTags)
	c.FromServer = make(chan *RPCReply)
	go c.IO()
	go c.readNetPackets()
	return c, nil
}

// GetTag gets a tag to be used to identify a message.
func (c *Client) GetTag() Tag {
	t := <-c.Tags
	if false {
		runtime.SetFinalizer(&t, func(t *Tag) {
			c.Tags <- *t
		})
	}
	return t
}

// GetFID gets a fid to be used to identify a resource for a 9p client.
// For a given lifetime of a 9p client, FIDS are unique (i.e. not reused as in
// many 9p client libraries).
func (c *Client) GetFID() FID {
	return FID(atomic.AddUint64(&c.FID, 1))
}

func (c *Client) readNetPackets() {
	if c.FromNet == nil {
		if c.Trace != nil {
			c.Trace("c.FromNet is nil, marking dead")
		}
		c.Dead = true
		return
	}
	defer c.FromNet.Close()
	defer close(c.FromServer)
	if c.Trace != nil {
		c.Trace("Starting readNetPackets")
	}
	for !c.Dead {
		l := make([]byte, 7)
		if c.Trace != nil {
			c.Trace("Before read")
		}

		if n, err := c.FromNet.Read(l); err != nil || n < 7 {
			log.Printf("readNetPackets: short read: %v", err)
			c.Dead = true
			return
		}
		if c.Trace != nil {
			c.Trace("Server reads %v", l)
		}
		s := int64(l[0]) + int64(l[1])<<8 + int64(l[2])<<16 + int64(l[3])<<24
		b := bytes.NewBuffer(l)
		r := io.LimitReader(c.FromNet, s-7)
		if _, err := io.Copy(b, r); err != nil {
			log.Printf("readNetPackets: short read: %v", err)
			c.Dead = true
			return
		}
		if c.Trace != nil {
			c.Trace("readNetPackets: got %v, len %d, sending to IO", RPCNames[MType(l[4])], b.Len())
		}
		c.FromServer <- &RPCReply{b: b.Bytes()}
	}
	if c.Trace != nil {
		c.Trace("Client %v is all done", c)
	}

}

func (c *Client) IO() {
	go func() {
		for {
			r := <-c.FromClient
			t := <-c.Tags
			if c.Trace != nil {
				c.Trace(fmt.Sprintf("Tag for request is %v", t))
			}
			r.b[5] = uint8(t)
			r.b[6] = uint8(t >> 8)
			if c.Trace != nil {
				c.Trace(fmt.Sprintf("Tag for request is %v", t))
			}
			c.RPC[int(t)-1] = r
			if c.Trace != nil {
				c.Trace("Write %v to ToNet", r.b)
			}
			if _, err := c.ToNet.Write(r.b); err != nil {
				c.Dead = true
				log.Fatalf("Write to server: %v", err)
				return
			}
		}
	}()

	for {
		r := <-c.FromServer
		if c.Trace != nil {
			c.Trace("Read %v FromServer", r.b)
		}
		t := Tag(r.b[5]) | Tag(r.b[6])<<8
		if c.Trace != nil {
			c.Trace(fmt.Sprintf("Tag for reply is %v", t))
		}
		if t < 1 {
			panic(fmt.Sprintf("tag %d < 1", t))
		}
		if int(t-1) >= len(c.RPC) {
			panic(fmt.Sprintf("tag %d >= len(c.RPC) %d", t, len(c.RPC)))
		}
		c.Trace("RPC %v ", c.RPC[t-1])
		rrr := c.RPC[t-1]
		c.Trace("rrr %v ", rrr)
		rrr.Reply <- r.b
		c.Tags <- t
	}
}

func (c *Client) String() string {
	z := map[bool]string{false: "Alive", true: "Dead"}
	return fmt.Sprintf("%v tags available, Msize %v, %v FromNet %v ToNet %v", len(c.Tags), c.Msize, z[c.Dead],
		c.FromNet, c.ToNet)
}
