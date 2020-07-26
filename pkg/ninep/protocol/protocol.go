// Package protocol implements the 9p protocol using the stubs.

//go:generate go run gen.go

package protocol

import "bytes"

// 9P2000 message types
const (
	Tversion MType = 100 + iota
	Rversion
	Tauth
	Rauth
	Tattach
	Rattach
	Terror
	Rerror
	Tflush
	Rflush
	Twalk
	Rwalk
	Topen
	Ropen
	Tcreate
	Rcreate
	Tread
	Rread
	Twrite
	Rwrite
	Tclunk
	Rclunk
	Tremove
	Rremove
	Tstat
	Rstat
	Twstat
	Rwstat
	Tlast
)

const (
	MSIZE   = 2*1048576 + IOHDRSZ // default message size (1048576+IOHdrSz)
	IOHDRSZ = 24                  // the non-data size of the Twrite messages
	PORT    = 564                 // default port for 9P file servers
	NumFID  = 1 << 16
	QIDLen  = 13
)

// QID types
const (
	QTDIR     = 0x80 // directories
	QTAPPEND  = 0x40 // append only files
	QTEXCL    = 0x20 // exclusive use files
	QTMOUNT   = 0x10 // mounted channel
	QTAUTH    = 0x08 // authentication file
	QTTMP     = 0x04 // non-backed-up file
	QTSYMLINK = 0x02 // symbolic link (Unix, 9P2000.u)
	QTLINK    = 0x01 // hard link (Unix, 9P2000.u)
	QTFILE    = 0x00
)

// Flags for the mode field in Topen and Tcreate messages
const (
	OREAD   = 0x0    // open read-only
	OWRITE  = 0x1    // open write-only
	ORDWR   = 0x2    // open read-write
	OEXEC   = 0x3    // execute (== read but check execute permission)
	OTRUNC  = 0x10   // or'ed in (except for exec), truncate file first
	OCEXEC  = 0x20   // or'ed in, close on exec
	ORCLOSE = 0x40   // or'ed in, remove on close
	OAPPEND = 0x80   // or'ed in, append only
	OEXCL   = 0x1000 // or'ed in, exclusive client use
)

// File modes
const (
	DMDIR    = 0x80000000 // mode bit for directories
	DMAPPEND = 0x40000000 // mode bit for append only files
	DMEXCL   = 0x20000000 // mode bit for exclusive use files
	DMMOUNT  = 0x10000000 // mode bit for mounted channel
	DMAUTH   = 0x08000000 // mode bit for authentication file
	DMTMP    = 0x04000000 // mode bit for non-backed-up file
	DMREAD   = 0x4        // mode bit for read permission
	DMWRITE  = 0x2        // mode bit for write permission
	DMEXEC   = 0x1        // mode bit for execute permission
)

const (
	NOTAG Tag = 0xFFFF     // no tag specified
	NOFID FID = 0xFFFFFFFF // no fid specified
	// We reserve tag NOTAG and tag 0. 0 is a troublesome value to pass
	// around, since it is also a default value and using it can hide errors
	// in the code.
	NumTags = 1<<16 - 2
)

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

// Types contained in 9p messages.
type (
	MType      uint8
	Mode       uint8
	NumEntries uint16
	Tag        uint16
	FID        uint32
	MaxSize    uint32
	Count      int32
	Perm       uint32
	Offset     uint64
	Data       []byte
	// Some []byte fields are encoded with a 16-bit length, e.g. stat data.
	// We use this type to tag such fields. The parameters are still []byte,
	// this was just the only way I could think of to make the stub generator do the right
	// thing.
	DataCnt16 byte // []byte with a 16-bit count.
)

// Error represents a 9P2000 error
type Error struct {
	Err string
}

// File identifier
type QID struct {
	Type    uint8  // type of the file (high 8 bits of the mode)
	Version uint32 // version number for the path
	Path    uint64 // server's unique identification of the file
}

// Dir describes a file
type Dir struct {
	Type    uint16
	Dev     uint32
	QID     QID    // file's QID
	Mode    uint32 // permissions and flags
	Atime   uint32 // last access time in seconds
	Mtime   uint32 // last modified time in seconds
	Length  uint64 // file length in bytes
	Name    string // file name
	User    string // owner name
	Group   string // group name
	ModUser string // name of the last user that modified the file
}

type Dispatcher func(s *Server, b *bytes.Buffer, t MType) error

// N.B. In all packets, the wire order is assumed to be the order in which you
// put struct members.
// In an earlier version of this code we got really fancy and made it so you
// could have identically named fields in the R and T packets. It's only an issue
// in a trivial number of packets so we place the burden on you, the user, to make
// the names different. Also, you can't name struct members with the same names as the
// type. Sorry. But it keeps gen.go so much simpler.

type TversionPkt struct {
	TMsize   MaxSize
	TVersion string
}

type RversionPkt struct {
	RMsize   MaxSize
	RVersion string
}

type TattachPkt struct {
	SFID  FID
	AFID  FID
	Uname string
	Aname string
}

type RattachPkt struct {
	QID QID
}

type TflushPkt struct {
	OTag Tag
}

type RflushPkt struct {
}

type TwalkPkt struct {
	SFID   FID
	NewFID FID
	Paths  []string
}

type RwalkPkt struct {
	QIDs []QID
}

type TopenPkt struct {
	OFID  FID
	Omode Mode
}

type RopenPkt struct {
	OQID   QID
	IOUnit MaxSize
}

type TcreatePkt struct {
	OFID       FID
	Name       string
	CreatePerm Perm
	Omode      Mode
}

type RcreatePkt struct {
	OQID   QID
	IOUnit MaxSize
}

type TclunkPkt struct {
	OFID FID
}

type RclunkPkt struct {
}

type TremovePkt struct {
	OFID FID
}

type RremovePkt struct {
}

type TstatPkt struct {
	OFID FID
}

type RstatPkt struct {
	B []DataCnt16
}

type TwstatPkt struct {
	OFID FID
	B    []DataCnt16
}

type RwstatPkt struct {
}

type TreadPkt struct {
	OFID FID
	Off  Offset
	Len  Count
}

type RreadPkt struct {
	Data []byte
}

type TwritePkt struct {
	OFID FID
	Off  Offset
	Data []byte
}

type RwritePkt struct {
	RLen Count
}

type RerrorPkt struct {
	Error string
}

type DirPkt struct {
	D Dir
}

type RPCCall struct {
	b     []byte
	Reply chan []byte
}

type RPCReply struct {
	b []byte
}

/* rpc servers */
type ClientOpt func(*Client) error
type ListenerOpt func(*Listener) error
type Tracer func(string, ...interface{})

type NineServer interface {
	Rversion(MaxSize, string) (MaxSize, string, error)
	Rattach(FID, FID, string, string) (QID, error)
	Rwalk(FID, FID, []string) ([]QID, error)
	Ropen(FID, Mode) (QID, MaxSize, error)
	Rcreate(FID, string, Perm, Mode) (QID, MaxSize, error)
	Rstat(FID) ([]byte, error)
	Rwstat(FID, []byte) error
	Rclunk(FID) error
	Rremove(FID) error
	Rread(FID, Offset, Count) ([]byte, error)
	Rwrite(FID, Offset, []byte) (Count, error)
	Rflush(Otag Tag) error
}

var (
	RPCNames = map[MType]string{
		Tversion: "Tversion",
		Rversion: "Rversion",
		Tauth:    "Tauth",
		Rauth:    "Rauth",
		Tattach:  "Tattach",
		Rattach:  "Rattach",
		Terror:   "Terror",
		Rerror:   "Rerror",
		Tflush:   "Tflush",
		Rflush:   "Rflush",
		Twalk:    "Twalk",
		Rwalk:    "Rwalk",
		Topen:    "Topen",
		Ropen:    "Ropen",
		Tcreate:  "Tcreate",
		Rcreate:  "Rcreate",
		Tread:    "Tread",
		Rread:    "Rread",
		Twrite:   "Twrite",
		Rwrite:   "Rwrite",
		Tclunk:   "Tclunk",
		Rclunk:   "Rclunk",
		Tremove:  "Tremove",
		Rremove:  "Rremove",
		Tstat:    "Tstat",
		Rstat:    "Rstat",
		Twstat:   "Twstat",
		Rwstat:   "Rwstat",
	}
)
