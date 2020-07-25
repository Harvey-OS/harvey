// Copyright 2009 The Ninep Authors.  All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

package protocol


// A File is defined by a QID. File Servers never see a FID.
// There are two channels. The first is for normal requests.
// The second is for Flushes. File server code always
// checks the flush channel first. At the same time, server code
// puts the flush into both channels, so the server code has some
// idea when the flush entered the queue. This is very similar
// to how MSI-X works on PCIe ...
type File struct {
}

/*
// A FileOp is a function to call, an abort channel, and a reply channel
type FileOp struct {
	f func() error
	abort chan abort
	reply chan
}
*/

// A service is a closure which returns an error or nil.
// It writes its result down the provided channel.
// It looks for flushes on the flushchan before doing its
// function, and will respond to all flushes while any are pending.
type Service func(func() error, chan FID)

// Server maintains file system server state. This is inclusive of RPC
// server state plus more. In our case when we walk to a fid we kick
// off a goroutine to manage it. As a result we need a map of Tag to FID
// so we know what to do about Tflush.
type FileServer struct {
	Server
	Versioned bool
}
