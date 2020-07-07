// +build plan9
package harvey

import "syscall"

type Perm uint32

// Size is the type we use to encode 9P2000
type Size uint32

// Tag is the id field used to identify Transmit Messgaes
type Tag uint16

// FID is the id of the current file.
type FID uint32

// Error values
const (
	EPERM   = 1
	ENOENT  = 2
	EIO     = 5
	EACCES  = 13
	EEXIST  = 17
	ENOTDIR = 20
	EINVAL  = 22
)

// QID types
const (
	QTDIR     = syscall.QTDIR    // directories
	QTAPPEND  = syscall.QTAPPEND // append only files
	QTEXCL    = syscall.QTEXCL   // exclusive use files
	QTMOUNT   = syscall.QTMOUNT  // mounted channel
	QTAUTH    = syscall.QTAUTH   // authentication file
	QTTMP     = syscall.QTTMP    // non-backed-up file
	QTSYMLINK = 0x02             // symbolic link (Unix, 9P2000.u)
	QTLINK    = 0x01             // hard link (Unix, 9P2000.u)
	QTFILE    = syscall.QTFILE
)

// Flags for the mode field in Topen and Tcreate messages
const (
	OREAD   = syscall.O_RDONLY // open read-only
	OWRITE  = syscall.O_WRONLY // open write-only
	ORDWR   = syscall.O_RDWR   // open read-write
	OEXEC   = 0x3              // execute (== read but check execute permission)
	OTRUNC  = 0x10             // or'ed in (except for exec), truncate file first
	OCEXEC  = 0x20             // or'ed in, close on exec
	ORCLOSE = 0x40             // or'ed in, remove on close
	OAPPEND = 0x80             // or'ed in, append only
	OEXCL   = syscall.O_EXCL   // or'ed in, exclusive client use
)

// File modes
const (
	DMDIR    = syscall.DMDIR    // mode bit for directories
	DMAPPEND = syscall.DMAPPEND // mode bit for append only files
	DMEXCL   = syscall.DMEXCL   // mode bit for exclusive use files
	DMMOUNT  = syscall.DMMOUNT  // mode bit for mounted channel
	DMAUTH   = syscall.DMAUTH   // mode bit for authentication file
	DMTMP    = syscall.DMTMP    // mode bit for non-backed-up file
	DMREAD   = syscall.DMREAD   // mode bit for read permission
	DMWRITE  = syscall.DMWRITE  // mode bit for write permission
	DMEXEC   = syscall.DMEXEC   // mode bit for execute permission
)

// A QID represents a 9P server's unique identification for a file.
type QID = syscall.QID

// A Dir contains the metadata for a file.
type Dir = syscall.Dir
