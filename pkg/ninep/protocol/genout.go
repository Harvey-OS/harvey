
package protocol
import (
"bytes"
"fmt"
_ "log"
)
func MarshalRerrorPkt (b *bytes.Buffer, t Tag, Error string) {
var l uint64
b.Reset()
b.Write([]byte{0,0,0,0,
uint8(Rerror),
byte(t), byte(t>>8),
	uint8(len(Error)),uint8(len(Error)>>8),
	})
	b.Write([]byte(Error))

{
l = uint64(b.Len())
copy(b.Bytes(), []byte{uint8(l), uint8(l>>8), uint8(l>>16), uint8(l>>24)})
}
return
}
func UnmarshalRerrorPkt (b *bytes.Buffer) (Error string,  t Tag, err error) {
var u [8]uint8
var l uint64
if _, err = b.Read(u[:2]); err != nil {
err = fmt.Errorf("pkt too short for Tag; need 2, have %d", b.Len())
return
}
l = uint64(u[0]) | uint64(u[1])<<8
t = Tag(l)
	if _, err = b.Read(u[:2]); err != nil {
		err = fmt.Errorf("pkt too short for uint16: need 2, have %d", b.Len())
	return
	}
	l = uint64(u[0])
	l |= uint64(u[1])<<8
	if b.Len() < int(l) {
		err = fmt.Errorf("pkt too short for string: need %d, have %d", l, b.Len())
	return
	}
	Error = string(b.Bytes()[:l])
	_ = b.Next(int(l))

if b.Len() > 0 {
err = fmt.Errorf("Packet too long: %d bytes left over after decode", b.Len())
}
return
}
func MarshalRversionPkt (b *bytes.Buffer, t Tag, RMsize MaxSize, RVersion string) {
var l uint64
b.Reset()
b.Write([]byte{0,0,0,0,
uint8(Rversion),
byte(t), byte(t>>8),
	uint8(RMsize>>0),
	uint8(RMsize>>8),
	uint8(RMsize>>16),
	uint8(RMsize>>24),
	uint8(len(RVersion)),uint8(len(RVersion)>>8),
	})
	b.Write([]byte(RVersion))

{
l = uint64(b.Len())
copy(b.Bytes(), []byte{uint8(l), uint8(l>>8), uint8(l>>16), uint8(l>>24)})
}
return
}
func UnmarshalRversionPkt (b *bytes.Buffer) (RMsize MaxSize, RVersion string,  t Tag, err error) {
var u [8]uint8
var l uint64
if _, err = b.Read(u[:2]); err != nil {
err = fmt.Errorf("pkt too short for Tag; need 2, have %d", b.Len())
return
}
l = uint64(u[0]) | uint64(u[1])<<8
t = Tag(l)
	if _, err = b.Read(u[:4]); err != nil {
		err = fmt.Errorf("pkt too short for uint32: need 4, have %d", b.Len())
	return
	}
	RMsize = MaxSize(u[0])
	RMsize |= MaxSize(u[1])<<8
	RMsize |= MaxSize(u[2])<<16
	RMsize |= MaxSize(u[3])<<24
	if _, err = b.Read(u[:2]); err != nil {
		err = fmt.Errorf("pkt too short for uint16: need 2, have %d", b.Len())
	return
	}
	l = uint64(u[0])
	l |= uint64(u[1])<<8
	if b.Len() < int(l) {
		err = fmt.Errorf("pkt too short for string: need %d, have %d", l, b.Len())
	return
	}
	RVersion = string(b.Bytes()[:l])
	_ = b.Next(int(l))

if b.Len() > 0 {
err = fmt.Errorf("Packet too long: %d bytes left over after decode", b.Len())
}
return
}
func MarshalTversionPkt (b *bytes.Buffer, t Tag, TMsize MaxSize, TVersion string) {
var l uint64
b.Reset()
b.Write([]byte{0,0,0,0,
uint8(Tversion),
byte(t), byte(t>>8),
	uint8(TMsize>>0),
	uint8(TMsize>>8),
	uint8(TMsize>>16),
	uint8(TMsize>>24),
	uint8(len(TVersion)),uint8(len(TVersion)>>8),
	})
	b.Write([]byte(TVersion))

{
l = uint64(b.Len())
copy(b.Bytes(), []byte{uint8(l), uint8(l>>8), uint8(l>>16), uint8(l>>24)})
}
return
}
func UnmarshalTversionPkt (b *bytes.Buffer) (TMsize MaxSize, TVersion string,  t Tag, err error) {
var u [8]uint8
var l uint64
if _, err = b.Read(u[:2]); err != nil {
err = fmt.Errorf("pkt too short for Tag; need 2, have %d", b.Len())
return
}
l = uint64(u[0]) | uint64(u[1])<<8
t = Tag(l)
	if _, err = b.Read(u[:4]); err != nil {
		err = fmt.Errorf("pkt too short for uint32: need 4, have %d", b.Len())
	return
	}
	TMsize = MaxSize(u[0])
	TMsize |= MaxSize(u[1])<<8
	TMsize |= MaxSize(u[2])<<16
	TMsize |= MaxSize(u[3])<<24
	if _, err = b.Read(u[:2]); err != nil {
		err = fmt.Errorf("pkt too short for uint16: need 2, have %d", b.Len())
	return
	}
	l = uint64(u[0])
	l |= uint64(u[1])<<8
	if b.Len() < int(l) {
		err = fmt.Errorf("pkt too short for string: need %d, have %d", l, b.Len())
	return
	}
	TVersion = string(b.Bytes()[:l])
	_ = b.Next(int(l))

if b.Len() > 0 {
err = fmt.Errorf("Packet too long: %d bytes left over after decode", b.Len())
}
return
}
func (s *Server) SrvRversion(b*bytes.Buffer) (err error) {
	TMsize, TVersion,  t, err := UnmarshalTversionPkt(b)
	//if err != nil {
	//}
	if RMsize, RVersion,  err := s.NS.Rversion(TMsize, TVersion); err != nil {
	MarshalRerrorPkt(b, t, fmt.Sprintf("%v", err))
} else {
	MarshalRversionPkt(b, t, RMsize, RVersion)
}
	return nil
}

func (c *Client)CallTversion (TMsize MaxSize, TVersion string) (RMsize MaxSize, RVersion string,  err error) {
var b = bytes.Buffer{}
if c.Trace != nil {c.Trace("%v", Tversion)}
t := Tag(0)
r := make (chan []byte)
if c.Trace != nil { c.Trace(":tag %v, FID %v", t, c.FID)}
MarshalTversionPkt(&b, t, TMsize, TVersion)
c.FromClient <- &RPCCall{b: b.Bytes(), Reply: r}
bb := <-r
if MType(bb[4]) == Rerror {
	s, _, err := UnmarshalRerrorPkt(bytes.NewBuffer(bb[5:]))
	if err != nil {
		return RMsize, RVersion,  err
	}
	return RMsize, RVersion,  fmt.Errorf("%v", s)
} else {
	RMsize, RVersion,  _, err = UnmarshalRversionPkt(bytes.NewBuffer(bb[5:]))
}
return RMsize, RVersion,  err
}
func MarshalRattachPkt (b *bytes.Buffer, t Tag, QID QID) {
var l uint64
b.Reset()
b.Write([]byte{0,0,0,0,
uint8(Rattach),
byte(t), byte(t>>8),
	uint8(QID.Type>>0),
	uint8(QID.Version>>0),
	uint8(QID.Version>>8),
	uint8(QID.Version>>16),
	uint8(QID.Version>>24),
	uint8(QID.Path>>0),
	uint8(QID.Path>>8),
	uint8(QID.Path>>16),
	uint8(QID.Path>>24),
	uint8(QID.Path>>32),
	uint8(QID.Path>>40),
	uint8(QID.Path>>48),
	uint8(QID.Path>>56),
	})

{
l = uint64(b.Len())
copy(b.Bytes(), []byte{uint8(l), uint8(l>>8), uint8(l>>16), uint8(l>>24)})
}
return
}
func UnmarshalRattachPkt (b *bytes.Buffer) (QID QID,  t Tag, err error) {
var u [8]uint8
var l uint64
if _, err = b.Read(u[:2]); err != nil {
err = fmt.Errorf("pkt too short for Tag; need 2, have %d", b.Len())
return
}
l = uint64(u[0]) | uint64(u[1])<<8
t = Tag(l)
	if _, err = b.Read(u[:1]); err != nil {
		err = fmt.Errorf("pkt too short for uint8: need 1, have %d", b.Len())
	return
	}
	QID.Type = uint8(u[0])
	if _, err = b.Read(u[:4]); err != nil {
		err = fmt.Errorf("pkt too short for uint32: need 4, have %d", b.Len())
	return
	}
	QID.Version = uint32(u[0])
	QID.Version |= uint32(u[1])<<8
	QID.Version |= uint32(u[2])<<16
	QID.Version |= uint32(u[3])<<24
	if _, err = b.Read(u[:8]); err != nil {
		err = fmt.Errorf("pkt too short for uint64: need 8, have %d", b.Len())
	return
	}
	QID.Path = uint64(u[0])
	QID.Path |= uint64(u[1])<<8
	QID.Path |= uint64(u[2])<<16
	QID.Path |= uint64(u[3])<<24
	QID.Path |= uint64(u[4])<<32
	QID.Path |= uint64(u[5])<<40
	QID.Path |= uint64(u[6])<<48
	QID.Path |= uint64(u[7])<<56

if b.Len() > 0 {
err = fmt.Errorf("Packet too long: %d bytes left over after decode", b.Len())
}
return
}
func MarshalTattachPkt (b *bytes.Buffer, t Tag, SFID FID, AFID FID, Uname string, Aname string) {
var l uint64
b.Reset()
b.Write([]byte{0,0,0,0,
uint8(Tattach),
byte(t), byte(t>>8),
	uint8(SFID>>0),
	uint8(SFID>>8),
	uint8(SFID>>16),
	uint8(SFID>>24),
	uint8(AFID>>0),
	uint8(AFID>>8),
	uint8(AFID>>16),
	uint8(AFID>>24),
	uint8(len(Uname)),uint8(len(Uname)>>8),
	})
	b.Write([]byte(Uname))
	b.Write([]byte{	uint8(len(Aname)),uint8(len(Aname)>>8),
	})
	b.Write([]byte(Aname))

{
l = uint64(b.Len())
copy(b.Bytes(), []byte{uint8(l), uint8(l>>8), uint8(l>>16), uint8(l>>24)})
}
return
}
func UnmarshalTattachPkt (b *bytes.Buffer) (SFID FID, AFID FID, Uname string, Aname string,  t Tag, err error) {
var u [8]uint8
var l uint64
if _, err = b.Read(u[:2]); err != nil {
err = fmt.Errorf("pkt too short for Tag; need 2, have %d", b.Len())
return
}
l = uint64(u[0]) | uint64(u[1])<<8
t = Tag(l)
	if _, err = b.Read(u[:4]); err != nil {
		err = fmt.Errorf("pkt too short for uint32: need 4, have %d", b.Len())
	return
	}
	SFID = FID(u[0])
	SFID |= FID(u[1])<<8
	SFID |= FID(u[2])<<16
	SFID |= FID(u[3])<<24
	if _, err = b.Read(u[:4]); err != nil {
		err = fmt.Errorf("pkt too short for uint32: need 4, have %d", b.Len())
	return
	}
	AFID = FID(u[0])
	AFID |= FID(u[1])<<8
	AFID |= FID(u[2])<<16
	AFID |= FID(u[3])<<24
	if _, err = b.Read(u[:2]); err != nil {
		err = fmt.Errorf("pkt too short for uint16: need 2, have %d", b.Len())
	return
	}
	l = uint64(u[0])
	l |= uint64(u[1])<<8
	if b.Len() < int(l) {
		err = fmt.Errorf("pkt too short for string: need %d, have %d", l, b.Len())
	return
	}
	Uname = string(b.Bytes()[:l])
	_ = b.Next(int(l))
	if _, err = b.Read(u[:2]); err != nil {
		err = fmt.Errorf("pkt too short for uint16: need 2, have %d", b.Len())
	return
	}
	l = uint64(u[0])
	l |= uint64(u[1])<<8
	if b.Len() < int(l) {
		err = fmt.Errorf("pkt too short for string: need %d, have %d", l, b.Len())
	return
	}
	Aname = string(b.Bytes()[:l])
	_ = b.Next(int(l))

if b.Len() > 0 {
err = fmt.Errorf("Packet too long: %d bytes left over after decode", b.Len())
}
return
}
func (s *Server) SrvRattach(b*bytes.Buffer) (err error) {
	SFID, AFID, Uname, Aname,  t, err := UnmarshalTattachPkt(b)
	//if err != nil {
	//}
	if QID,  err := s.NS.Rattach(SFID, AFID, Uname, Aname); err != nil {
	MarshalRerrorPkt(b, t, fmt.Sprintf("%v", err))
} else {
	MarshalRattachPkt(b, t, QID)
}
	return nil
}

func (c *Client)CallTattach (SFID FID, AFID FID, Uname string, Aname string) (QID QID,  err error) {
var b = bytes.Buffer{}
if c.Trace != nil {c.Trace("%v", Tattach)}
t := Tag(0)
r := make (chan []byte)
if c.Trace != nil { c.Trace(":tag %v, FID %v", t, c.FID)}
MarshalTattachPkt(&b, t, SFID, AFID, Uname, Aname)
c.FromClient <- &RPCCall{b: b.Bytes(), Reply: r}
bb := <-r
if MType(bb[4]) == Rerror {
	s, _, err := UnmarshalRerrorPkt(bytes.NewBuffer(bb[5:]))
	if err != nil {
		return QID,  err
	}
	return QID,  fmt.Errorf("%v", s)
} else {
	QID,  _, err = UnmarshalRattachPkt(bytes.NewBuffer(bb[5:]))
}
return QID,  err
}
func MarshalRflushPkt (b *bytes.Buffer, t Tag, ) {
var l uint64
b.Reset()
b.Write([]byte{0,0,0,0,
uint8(Rflush),
byte(t), byte(t>>8),
	})

{
l = uint64(b.Len())
copy(b.Bytes(), []byte{uint8(l), uint8(l>>8), uint8(l>>16), uint8(l>>24)})
}
return
}
func UnmarshalRflushPkt (b *bytes.Buffer) ( t Tag, err error) {
var u [8]uint8
var l uint64
if _, err = b.Read(u[:2]); err != nil {
err = fmt.Errorf("pkt too short for Tag; need 2, have %d", b.Len())
return
}
l = uint64(u[0]) | uint64(u[1])<<8
t = Tag(l)

if b.Len() > 0 {
err = fmt.Errorf("Packet too long: %d bytes left over after decode", b.Len())
}
return
}
func MarshalTflushPkt (b *bytes.Buffer, t Tag, OTag Tag) {
var l uint64
b.Reset()
b.Write([]byte{0,0,0,0,
uint8(Tflush),
byte(t), byte(t>>8),
	uint8(OTag>>0),
	uint8(OTag>>8),
	})

{
l = uint64(b.Len())
copy(b.Bytes(), []byte{uint8(l), uint8(l>>8), uint8(l>>16), uint8(l>>24)})
}
return
}
func UnmarshalTflushPkt (b *bytes.Buffer) (OTag Tag,  t Tag, err error) {
var u [8]uint8
var l uint64
if _, err = b.Read(u[:2]); err != nil {
err = fmt.Errorf("pkt too short for Tag; need 2, have %d", b.Len())
return
}
l = uint64(u[0]) | uint64(u[1])<<8
t = Tag(l)
	if _, err = b.Read(u[:2]); err != nil {
		err = fmt.Errorf("pkt too short for uint16: need 2, have %d", b.Len())
	return
	}
	OTag = Tag(u[0])
	OTag |= Tag(u[1])<<8

if b.Len() > 0 {
err = fmt.Errorf("Packet too long: %d bytes left over after decode", b.Len())
}
return
}
func (s *Server) SrvRflush(b*bytes.Buffer) (err error) {
	OTag,  t, err := UnmarshalTflushPkt(b)
	//if err != nil {
	//}
	if  err := s.NS.Rflush(OTag); err != nil {
	MarshalRerrorPkt(b, t, fmt.Sprintf("%v", err))
} else {
	MarshalRflushPkt(b, t, )
}
	return nil
}

func (c *Client)CallTflush (OTag Tag) ( err error) {
var b = bytes.Buffer{}
if c.Trace != nil {c.Trace("%v", Tflush)}
t := Tag(0)
r := make (chan []byte)
if c.Trace != nil { c.Trace(":tag %v, FID %v", t, c.FID)}
MarshalTflushPkt(&b, t, OTag)
c.FromClient <- &RPCCall{b: b.Bytes(), Reply: r}
bb := <-r
if MType(bb[4]) == Rerror {
	s, _, err := UnmarshalRerrorPkt(bytes.NewBuffer(bb[5:]))
	if err != nil {
		return  err
	}
	return  fmt.Errorf("%v", s)
} else {
	 _, err = UnmarshalRflushPkt(bytes.NewBuffer(bb[5:]))
}
return  err
}
func MarshalRwalkPkt (b *bytes.Buffer, t Tag, QIDs []QID) {
var l uint64
b.Reset()
b.Write([]byte{0,0,0,0,
uint8(Rwalk),
byte(t), byte(t>>8),
	uint8(len(QIDs)>>0),
	uint8(len(QIDs)>>8),
	})
for i := range QIDs {
	b.Write([]byte{	uint8(QIDs[i].Type>>0),
	uint8(QIDs[i].Version>>0),
	uint8(QIDs[i].Version>>8),
	uint8(QIDs[i].Version>>16),
	uint8(QIDs[i].Version>>24),
	uint8(QIDs[i].Path>>0),
	uint8(QIDs[i].Path>>8),
	uint8(QIDs[i].Path>>16),
	uint8(QIDs[i].Path>>24),
	uint8(QIDs[i].Path>>32),
	uint8(QIDs[i].Path>>40),
	uint8(QIDs[i].Path>>48),
	uint8(QIDs[i].Path>>56),
	})
}

{
l = uint64(b.Len())
copy(b.Bytes(), []byte{uint8(l), uint8(l>>8), uint8(l>>16), uint8(l>>24)})
}
return
}
func UnmarshalRwalkPkt (b *bytes.Buffer) (QIDs []QID,  t Tag, err error) {
var u [8]uint8
var l uint64
if _, err = b.Read(u[:2]); err != nil {
err = fmt.Errorf("pkt too short for Tag; need 2, have %d", b.Len())
return
}
l = uint64(u[0]) | uint64(u[1])<<8
t = Tag(l)
	if _, err = b.Read(u[:2]); err != nil {
		err = fmt.Errorf("pkt too short for uint16: need 2, have %d", b.Len())
	return
	}
	l = uint64(u[0])
	l |= uint64(u[1])<<8
	QIDs = make([]QID, l)
for i := range QIDs {
	if _, err = b.Read(u[:1]); err != nil {
		err = fmt.Errorf("pkt too short for uint8: need 1, have %d", b.Len())
	return
	}
	QIDs[i].Type = uint8(u[0])
	if _, err = b.Read(u[:4]); err != nil {
		err = fmt.Errorf("pkt too short for uint32: need 4, have %d", b.Len())
	return
	}
	QIDs[i].Version = uint32(u[0])
	QIDs[i].Version |= uint32(u[1])<<8
	QIDs[i].Version |= uint32(u[2])<<16
	QIDs[i].Version |= uint32(u[3])<<24
	if _, err = b.Read(u[:8]); err != nil {
		err = fmt.Errorf("pkt too short for uint64: need 8, have %d", b.Len())
	return
	}
	QIDs[i].Path = uint64(u[0])
	QIDs[i].Path |= uint64(u[1])<<8
	QIDs[i].Path |= uint64(u[2])<<16
	QIDs[i].Path |= uint64(u[3])<<24
	QIDs[i].Path |= uint64(u[4])<<32
	QIDs[i].Path |= uint64(u[5])<<40
	QIDs[i].Path |= uint64(u[6])<<48
	QIDs[i].Path |= uint64(u[7])<<56
}

if b.Len() > 0 {
err = fmt.Errorf("Packet too long: %d bytes left over after decode", b.Len())
}
return
}
func MarshalTwalkPkt (b *bytes.Buffer, t Tag, SFID FID, NewFID FID, Paths []string) {
var l uint64
b.Reset()
b.Write([]byte{0,0,0,0,
uint8(Twalk),
byte(t), byte(t>>8),
	uint8(SFID>>0),
	uint8(SFID>>8),
	uint8(SFID>>16),
	uint8(SFID>>24),
	uint8(NewFID>>0),
	uint8(NewFID>>8),
	uint8(NewFID>>16),
	uint8(NewFID>>24),
	uint8(len(Paths)>>0),
	uint8(len(Paths)>>8),
	})
for i := range Paths {
	b.Write([]byte{	uint8(len(Paths[i])),uint8(len(Paths[i])>>8),
	})
	b.Write([]byte(Paths[i]))
}

{
l = uint64(b.Len())
copy(b.Bytes(), []byte{uint8(l), uint8(l>>8), uint8(l>>16), uint8(l>>24)})
}
return
}
func UnmarshalTwalkPkt (b *bytes.Buffer) (SFID FID, NewFID FID, Paths []string,  t Tag, err error) {
var u [8]uint8
var l uint64
if _, err = b.Read(u[:2]); err != nil {
err = fmt.Errorf("pkt too short for Tag; need 2, have %d", b.Len())
return
}
l = uint64(u[0]) | uint64(u[1])<<8
t = Tag(l)
	if _, err = b.Read(u[:4]); err != nil {
		err = fmt.Errorf("pkt too short for uint32: need 4, have %d", b.Len())
	return
	}
	SFID = FID(u[0])
	SFID |= FID(u[1])<<8
	SFID |= FID(u[2])<<16
	SFID |= FID(u[3])<<24
	if _, err = b.Read(u[:4]); err != nil {
		err = fmt.Errorf("pkt too short for uint32: need 4, have %d", b.Len())
	return
	}
	NewFID = FID(u[0])
	NewFID |= FID(u[1])<<8
	NewFID |= FID(u[2])<<16
	NewFID |= FID(u[3])<<24
	if _, err = b.Read(u[:2]); err != nil {
		err = fmt.Errorf("pkt too short for uint16: need 2, have %d", b.Len())
	return
	}
	l = uint64(u[0])
	l |= uint64(u[1])<<8
	Paths = make([]string, l)
for i := range Paths {
	if _, err = b.Read(u[:2]); err != nil {
		err = fmt.Errorf("pkt too short for uint16: need 2, have %d", b.Len())
	return
	}
	l = uint64(u[0])
	l |= uint64(u[1])<<8
	if b.Len() < int(l) {
		err = fmt.Errorf("pkt too short for string: need %d, have %d", l, b.Len())
	return
	}
	Paths[i] = string(b.Bytes()[:l])
	_ = b.Next(int(l))
}

if b.Len() > 0 {
err = fmt.Errorf("Packet too long: %d bytes left over after decode", b.Len())
}
return
}
func (s *Server) SrvRwalk(b*bytes.Buffer) (err error) {
	SFID, NewFID, Paths,  t, err := UnmarshalTwalkPkt(b)
	//if err != nil {
	//}
	if QIDs,  err := s.NS.Rwalk(SFID, NewFID, Paths); err != nil {
	MarshalRerrorPkt(b, t, fmt.Sprintf("%v", err))
} else {
	MarshalRwalkPkt(b, t, QIDs)
}
	return nil
}

func (c *Client)CallTwalk (SFID FID, NewFID FID, Paths []string) (QIDs []QID,  err error) {
var b = bytes.Buffer{}
if c.Trace != nil {c.Trace("%v", Twalk)}
t := Tag(0)
r := make (chan []byte)
if c.Trace != nil { c.Trace(":tag %v, FID %v", t, c.FID)}
MarshalTwalkPkt(&b, t, SFID, NewFID, Paths)
c.FromClient <- &RPCCall{b: b.Bytes(), Reply: r}
bb := <-r
if MType(bb[4]) == Rerror {
	s, _, err := UnmarshalRerrorPkt(bytes.NewBuffer(bb[5:]))
	if err != nil {
		return QIDs,  err
	}
	return QIDs,  fmt.Errorf("%v", s)
} else {
	QIDs,  _, err = UnmarshalRwalkPkt(bytes.NewBuffer(bb[5:]))
}
return QIDs,  err
}
func MarshalRopenPkt (b *bytes.Buffer, t Tag, OQID QID, IOUnit MaxSize) {
var l uint64
b.Reset()
b.Write([]byte{0,0,0,0,
uint8(Ropen),
byte(t), byte(t>>8),
	uint8(OQID.Type>>0),
	uint8(OQID.Version>>0),
	uint8(OQID.Version>>8),
	uint8(OQID.Version>>16),
	uint8(OQID.Version>>24),
	uint8(OQID.Path>>0),
	uint8(OQID.Path>>8),
	uint8(OQID.Path>>16),
	uint8(OQID.Path>>24),
	uint8(OQID.Path>>32),
	uint8(OQID.Path>>40),
	uint8(OQID.Path>>48),
	uint8(OQID.Path>>56),
	uint8(IOUnit>>0),
	uint8(IOUnit>>8),
	uint8(IOUnit>>16),
	uint8(IOUnit>>24),
	})

{
l = uint64(b.Len())
copy(b.Bytes(), []byte{uint8(l), uint8(l>>8), uint8(l>>16), uint8(l>>24)})
}
return
}
func UnmarshalRopenPkt (b *bytes.Buffer) (OQID QID, IOUnit MaxSize,  t Tag, err error) {
var u [8]uint8
var l uint64
if _, err = b.Read(u[:2]); err != nil {
err = fmt.Errorf("pkt too short for Tag; need 2, have %d", b.Len())
return
}
l = uint64(u[0]) | uint64(u[1])<<8
t = Tag(l)
	if _, err = b.Read(u[:1]); err != nil {
		err = fmt.Errorf("pkt too short for uint8: need 1, have %d", b.Len())
	return
	}
	OQID.Type = uint8(u[0])
	if _, err = b.Read(u[:4]); err != nil {
		err = fmt.Errorf("pkt too short for uint32: need 4, have %d", b.Len())
	return
	}
	OQID.Version = uint32(u[0])
	OQID.Version |= uint32(u[1])<<8
	OQID.Version |= uint32(u[2])<<16
	OQID.Version |= uint32(u[3])<<24
	if _, err = b.Read(u[:8]); err != nil {
		err = fmt.Errorf("pkt too short for uint64: need 8, have %d", b.Len())
	return
	}
	OQID.Path = uint64(u[0])
	OQID.Path |= uint64(u[1])<<8
	OQID.Path |= uint64(u[2])<<16
	OQID.Path |= uint64(u[3])<<24
	OQID.Path |= uint64(u[4])<<32
	OQID.Path |= uint64(u[5])<<40
	OQID.Path |= uint64(u[6])<<48
	OQID.Path |= uint64(u[7])<<56
	if _, err = b.Read(u[:4]); err != nil {
		err = fmt.Errorf("pkt too short for uint32: need 4, have %d", b.Len())
	return
	}
	IOUnit = MaxSize(u[0])
	IOUnit |= MaxSize(u[1])<<8
	IOUnit |= MaxSize(u[2])<<16
	IOUnit |= MaxSize(u[3])<<24

if b.Len() > 0 {
err = fmt.Errorf("Packet too long: %d bytes left over after decode", b.Len())
}
return
}
func MarshalTopenPkt (b *bytes.Buffer, t Tag, OFID FID, Omode Mode) {
var l uint64
b.Reset()
b.Write([]byte{0,0,0,0,
uint8(Topen),
byte(t), byte(t>>8),
	uint8(OFID>>0),
	uint8(OFID>>8),
	uint8(OFID>>16),
	uint8(OFID>>24),
	uint8(Omode>>0),
	})

{
l = uint64(b.Len())
copy(b.Bytes(), []byte{uint8(l), uint8(l>>8), uint8(l>>16), uint8(l>>24)})
}
return
}
func UnmarshalTopenPkt (b *bytes.Buffer) (OFID FID, Omode Mode,  t Tag, err error) {
var u [8]uint8
var l uint64
if _, err = b.Read(u[:2]); err != nil {
err = fmt.Errorf("pkt too short for Tag; need 2, have %d", b.Len())
return
}
l = uint64(u[0]) | uint64(u[1])<<8
t = Tag(l)
	if _, err = b.Read(u[:4]); err != nil {
		err = fmt.Errorf("pkt too short for uint32: need 4, have %d", b.Len())
	return
	}
	OFID = FID(u[0])
	OFID |= FID(u[1])<<8
	OFID |= FID(u[2])<<16
	OFID |= FID(u[3])<<24
	if _, err = b.Read(u[:1]); err != nil {
		err = fmt.Errorf("pkt too short for uint8: need 1, have %d", b.Len())
	return
	}
	Omode = Mode(u[0])

if b.Len() > 0 {
err = fmt.Errorf("Packet too long: %d bytes left over after decode", b.Len())
}
return
}
func (s *Server) SrvRopen(b*bytes.Buffer) (err error) {
	OFID, Omode,  t, err := UnmarshalTopenPkt(b)
	//if err != nil {
	//}
	if OQID, IOUnit,  err := s.NS.Ropen(OFID, Omode); err != nil {
	MarshalRerrorPkt(b, t, fmt.Sprintf("%v", err))
} else {
	MarshalRopenPkt(b, t, OQID, IOUnit)
}
	return nil
}

func (c *Client)CallTopen (OFID FID, Omode Mode) (OQID QID, IOUnit MaxSize,  err error) {
var b = bytes.Buffer{}
if c.Trace != nil {c.Trace("%v", Topen)}
t := Tag(0)
r := make (chan []byte)
if c.Trace != nil { c.Trace(":tag %v, FID %v", t, c.FID)}
MarshalTopenPkt(&b, t, OFID, Omode)
c.FromClient <- &RPCCall{b: b.Bytes(), Reply: r}
bb := <-r
if MType(bb[4]) == Rerror {
	s, _, err := UnmarshalRerrorPkt(bytes.NewBuffer(bb[5:]))
	if err != nil {
		return OQID, IOUnit,  err
	}
	return OQID, IOUnit,  fmt.Errorf("%v", s)
} else {
	OQID, IOUnit,  _, err = UnmarshalRopenPkt(bytes.NewBuffer(bb[5:]))
}
return OQID, IOUnit,  err
}
func MarshalRcreatePkt (b *bytes.Buffer, t Tag, OQID QID, IOUnit MaxSize) {
var l uint64
b.Reset()
b.Write([]byte{0,0,0,0,
uint8(Rcreate),
byte(t), byte(t>>8),
	uint8(OQID.Type>>0),
	uint8(OQID.Version>>0),
	uint8(OQID.Version>>8),
	uint8(OQID.Version>>16),
	uint8(OQID.Version>>24),
	uint8(OQID.Path>>0),
	uint8(OQID.Path>>8),
	uint8(OQID.Path>>16),
	uint8(OQID.Path>>24),
	uint8(OQID.Path>>32),
	uint8(OQID.Path>>40),
	uint8(OQID.Path>>48),
	uint8(OQID.Path>>56),
	uint8(IOUnit>>0),
	uint8(IOUnit>>8),
	uint8(IOUnit>>16),
	uint8(IOUnit>>24),
	})

{
l = uint64(b.Len())
copy(b.Bytes(), []byte{uint8(l), uint8(l>>8), uint8(l>>16), uint8(l>>24)})
}
return
}
func UnmarshalRcreatePkt (b *bytes.Buffer) (OQID QID, IOUnit MaxSize,  t Tag, err error) {
var u [8]uint8
var l uint64
if _, err = b.Read(u[:2]); err != nil {
err = fmt.Errorf("pkt too short for Tag; need 2, have %d", b.Len())
return
}
l = uint64(u[0]) | uint64(u[1])<<8
t = Tag(l)
	if _, err = b.Read(u[:1]); err != nil {
		err = fmt.Errorf("pkt too short for uint8: need 1, have %d", b.Len())
	return
	}
	OQID.Type = uint8(u[0])
	if _, err = b.Read(u[:4]); err != nil {
		err = fmt.Errorf("pkt too short for uint32: need 4, have %d", b.Len())
	return
	}
	OQID.Version = uint32(u[0])
	OQID.Version |= uint32(u[1])<<8
	OQID.Version |= uint32(u[2])<<16
	OQID.Version |= uint32(u[3])<<24
	if _, err = b.Read(u[:8]); err != nil {
		err = fmt.Errorf("pkt too short for uint64: need 8, have %d", b.Len())
	return
	}
	OQID.Path = uint64(u[0])
	OQID.Path |= uint64(u[1])<<8
	OQID.Path |= uint64(u[2])<<16
	OQID.Path |= uint64(u[3])<<24
	OQID.Path |= uint64(u[4])<<32
	OQID.Path |= uint64(u[5])<<40
	OQID.Path |= uint64(u[6])<<48
	OQID.Path |= uint64(u[7])<<56
	if _, err = b.Read(u[:4]); err != nil {
		err = fmt.Errorf("pkt too short for uint32: need 4, have %d", b.Len())
	return
	}
	IOUnit = MaxSize(u[0])
	IOUnit |= MaxSize(u[1])<<8
	IOUnit |= MaxSize(u[2])<<16
	IOUnit |= MaxSize(u[3])<<24

if b.Len() > 0 {
err = fmt.Errorf("Packet too long: %d bytes left over after decode", b.Len())
}
return
}
func MarshalTcreatePkt (b *bytes.Buffer, t Tag, OFID FID, Name string, CreatePerm Perm, Omode Mode) {
var l uint64
b.Reset()
b.Write([]byte{0,0,0,0,
uint8(Tcreate),
byte(t), byte(t>>8),
	uint8(OFID>>0),
	uint8(OFID>>8),
	uint8(OFID>>16),
	uint8(OFID>>24),
	uint8(len(Name)),uint8(len(Name)>>8),
	})
	b.Write([]byte(Name))
	b.Write([]byte{	uint8(CreatePerm>>0),
	uint8(CreatePerm>>8),
	uint8(CreatePerm>>16),
	uint8(CreatePerm>>24),
	uint8(Omode>>0),
	})

{
l = uint64(b.Len())
copy(b.Bytes(), []byte{uint8(l), uint8(l>>8), uint8(l>>16), uint8(l>>24)})
}
return
}
func UnmarshalTcreatePkt (b *bytes.Buffer) (OFID FID, Name string, CreatePerm Perm, Omode Mode,  t Tag, err error) {
var u [8]uint8
var l uint64
if _, err = b.Read(u[:2]); err != nil {
err = fmt.Errorf("pkt too short for Tag; need 2, have %d", b.Len())
return
}
l = uint64(u[0]) | uint64(u[1])<<8
t = Tag(l)
	if _, err = b.Read(u[:4]); err != nil {
		err = fmt.Errorf("pkt too short for uint32: need 4, have %d", b.Len())
	return
	}
	OFID = FID(u[0])
	OFID |= FID(u[1])<<8
	OFID |= FID(u[2])<<16
	OFID |= FID(u[3])<<24
	if _, err = b.Read(u[:2]); err != nil {
		err = fmt.Errorf("pkt too short for uint16: need 2, have %d", b.Len())
	return
	}
	l = uint64(u[0])
	l |= uint64(u[1])<<8
	if b.Len() < int(l) {
		err = fmt.Errorf("pkt too short for string: need %d, have %d", l, b.Len())
	return
	}
	Name = string(b.Bytes()[:l])
	_ = b.Next(int(l))
	if _, err = b.Read(u[:4]); err != nil {
		err = fmt.Errorf("pkt too short for uint32: need 4, have %d", b.Len())
	return
	}
	CreatePerm = Perm(u[0])
	CreatePerm |= Perm(u[1])<<8
	CreatePerm |= Perm(u[2])<<16
	CreatePerm |= Perm(u[3])<<24
	if _, err = b.Read(u[:1]); err != nil {
		err = fmt.Errorf("pkt too short for uint8: need 1, have %d", b.Len())
	return
	}
	Omode = Mode(u[0])

if b.Len() > 0 {
err = fmt.Errorf("Packet too long: %d bytes left over after decode", b.Len())
}
return
}
func (s *Server) SrvRcreate(b*bytes.Buffer) (err error) {
	OFID, Name, CreatePerm, Omode,  t, err := UnmarshalTcreatePkt(b)
	//if err != nil {
	//}
	if OQID, IOUnit,  err := s.NS.Rcreate(OFID, Name, CreatePerm, Omode); err != nil {
	MarshalRerrorPkt(b, t, fmt.Sprintf("%v", err))
} else {
	MarshalRcreatePkt(b, t, OQID, IOUnit)
}
	return nil
}

func (c *Client)CallTcreate (OFID FID, Name string, CreatePerm Perm, Omode Mode) (OQID QID, IOUnit MaxSize,  err error) {
var b = bytes.Buffer{}
if c.Trace != nil {c.Trace("%v", Tcreate)}
t := Tag(0)
r := make (chan []byte)
if c.Trace != nil { c.Trace(":tag %v, FID %v", t, c.FID)}
MarshalTcreatePkt(&b, t, OFID, Name, CreatePerm, Omode)
c.FromClient <- &RPCCall{b: b.Bytes(), Reply: r}
bb := <-r
if MType(bb[4]) == Rerror {
	s, _, err := UnmarshalRerrorPkt(bytes.NewBuffer(bb[5:]))
	if err != nil {
		return OQID, IOUnit,  err
	}
	return OQID, IOUnit,  fmt.Errorf("%v", s)
} else {
	OQID, IOUnit,  _, err = UnmarshalRcreatePkt(bytes.NewBuffer(bb[5:]))
}
return OQID, IOUnit,  err
}
func MarshalRstatPkt (b *bytes.Buffer, t Tag, B []byte) {
var l uint64
b.Reset()
b.Write([]byte{0,0,0,0,
uint8(Rstat),
byte(t), byte(t>>8),
	uint8(len(B)>>0),
	uint8(len(B)>>8),
	})
	b.Write(B)

{
l = uint64(b.Len())
copy(b.Bytes(), []byte{uint8(l), uint8(l>>8), uint8(l>>16), uint8(l>>24)})
}
return
}
func UnmarshalRstatPkt (b *bytes.Buffer) (B []byte,  t Tag, err error) {
var u [8]uint8
var l uint64
if _, err = b.Read(u[:2]); err != nil {
err = fmt.Errorf("pkt too short for Tag; need 2, have %d", b.Len())
return
}
l = uint64(u[0]) | uint64(u[1])<<8
t = Tag(l)
	if _, err = b.Read(u[:2]); err != nil {
		err = fmt.Errorf("pkt too short for uint16: need 2, have %d", b.Len())
	return
	}
	l = uint64(u[0])
	l |= uint64(u[1])<<8
	B = b.Bytes()[:l]
	_ = b.Next(int(l))

if b.Len() > 0 {
err = fmt.Errorf("Packet too long: %d bytes left over after decode", b.Len())
}
return
}
func MarshalTstatPkt (b *bytes.Buffer, t Tag, OFID FID) {
var l uint64
b.Reset()
b.Write([]byte{0,0,0,0,
uint8(Tstat),
byte(t), byte(t>>8),
	uint8(OFID>>0),
	uint8(OFID>>8),
	uint8(OFID>>16),
	uint8(OFID>>24),
	})

{
l = uint64(b.Len())
copy(b.Bytes(), []byte{uint8(l), uint8(l>>8), uint8(l>>16), uint8(l>>24)})
}
return
}
func UnmarshalTstatPkt (b *bytes.Buffer) (OFID FID,  t Tag, err error) {
var u [8]uint8
var l uint64
if _, err = b.Read(u[:2]); err != nil {
err = fmt.Errorf("pkt too short for Tag; need 2, have %d", b.Len())
return
}
l = uint64(u[0]) | uint64(u[1])<<8
t = Tag(l)
	if _, err = b.Read(u[:4]); err != nil {
		err = fmt.Errorf("pkt too short for uint32: need 4, have %d", b.Len())
	return
	}
	OFID = FID(u[0])
	OFID |= FID(u[1])<<8
	OFID |= FID(u[2])<<16
	OFID |= FID(u[3])<<24

if b.Len() > 0 {
err = fmt.Errorf("Packet too long: %d bytes left over after decode", b.Len())
}
return
}
func (s *Server) SrvRstat(b*bytes.Buffer) (err error) {
	OFID,  t, err := UnmarshalTstatPkt(b)
	//if err != nil {
	//}
	if B,  err := s.NS.Rstat(OFID); err != nil {
	MarshalRerrorPkt(b, t, fmt.Sprintf("%v", err))
} else {
	MarshalRstatPkt(b, t, B)
}
	return nil
}

func (c *Client)CallTstat (OFID FID) (B []byte,  err error) {
var b = bytes.Buffer{}
if c.Trace != nil {c.Trace("%v", Tstat)}
t := Tag(0)
r := make (chan []byte)
if c.Trace != nil { c.Trace(":tag %v, FID %v", t, c.FID)}
MarshalTstatPkt(&b, t, OFID)
c.FromClient <- &RPCCall{b: b.Bytes(), Reply: r}
bb := <-r
if MType(bb[4]) == Rerror {
	s, _, err := UnmarshalRerrorPkt(bytes.NewBuffer(bb[5:]))
	if err != nil {
		return B,  err
	}
	return B,  fmt.Errorf("%v", s)
} else {
	B,  _, err = UnmarshalRstatPkt(bytes.NewBuffer(bb[5:]))
}
return B,  err
}
func MarshalRwstatPkt (b *bytes.Buffer, t Tag, ) {
var l uint64
b.Reset()
b.Write([]byte{0,0,0,0,
uint8(Rwstat),
byte(t), byte(t>>8),
	})

{
l = uint64(b.Len())
copy(b.Bytes(), []byte{uint8(l), uint8(l>>8), uint8(l>>16), uint8(l>>24)})
}
return
}
func UnmarshalRwstatPkt (b *bytes.Buffer) ( t Tag, err error) {
var u [8]uint8
var l uint64
if _, err = b.Read(u[:2]); err != nil {
err = fmt.Errorf("pkt too short for Tag; need 2, have %d", b.Len())
return
}
l = uint64(u[0]) | uint64(u[1])<<8
t = Tag(l)

if b.Len() > 0 {
err = fmt.Errorf("Packet too long: %d bytes left over after decode", b.Len())
}
return
}
func MarshalTwstatPkt (b *bytes.Buffer, t Tag, OFID FID, B []byte) {
var l uint64
b.Reset()
b.Write([]byte{0,0,0,0,
uint8(Twstat),
byte(t), byte(t>>8),
	uint8(OFID>>0),
	uint8(OFID>>8),
	uint8(OFID>>16),
	uint8(OFID>>24),
	uint8(len(B)>>0),
	uint8(len(B)>>8),
	})
	b.Write(B)

{
l = uint64(b.Len())
copy(b.Bytes(), []byte{uint8(l), uint8(l>>8), uint8(l>>16), uint8(l>>24)})
}
return
}
func UnmarshalTwstatPkt (b *bytes.Buffer) (OFID FID, B []byte,  t Tag, err error) {
var u [8]uint8
var l uint64
if _, err = b.Read(u[:2]); err != nil {
err = fmt.Errorf("pkt too short for Tag; need 2, have %d", b.Len())
return
}
l = uint64(u[0]) | uint64(u[1])<<8
t = Tag(l)
	if _, err = b.Read(u[:4]); err != nil {
		err = fmt.Errorf("pkt too short for uint32: need 4, have %d", b.Len())
	return
	}
	OFID = FID(u[0])
	OFID |= FID(u[1])<<8
	OFID |= FID(u[2])<<16
	OFID |= FID(u[3])<<24
	if _, err = b.Read(u[:2]); err != nil {
		err = fmt.Errorf("pkt too short for uint16: need 2, have %d", b.Len())
	return
	}
	l = uint64(u[0])
	l |= uint64(u[1])<<8
	B = b.Bytes()[:l]
	_ = b.Next(int(l))

if b.Len() > 0 {
err = fmt.Errorf("Packet too long: %d bytes left over after decode", b.Len())
}
return
}
func (s *Server) SrvRwstat(b*bytes.Buffer) (err error) {
	OFID, B,  t, err := UnmarshalTwstatPkt(b)
	//if err != nil {
	//}
	if  err := s.NS.Rwstat(OFID, B); err != nil {
	MarshalRerrorPkt(b, t, fmt.Sprintf("%v", err))
} else {
	MarshalRwstatPkt(b, t, )
}
	return nil
}

func (c *Client)CallTwstat (OFID FID, B []byte) ( err error) {
var b = bytes.Buffer{}
if c.Trace != nil {c.Trace("%v", Twstat)}
t := Tag(0)
r := make (chan []byte)
if c.Trace != nil { c.Trace(":tag %v, FID %v", t, c.FID)}
MarshalTwstatPkt(&b, t, OFID, B)
c.FromClient <- &RPCCall{b: b.Bytes(), Reply: r}
bb := <-r
if MType(bb[4]) == Rerror {
	s, _, err := UnmarshalRerrorPkt(bytes.NewBuffer(bb[5:]))
	if err != nil {
		return  err
	}
	return  fmt.Errorf("%v", s)
} else {
	 _, err = UnmarshalRwstatPkt(bytes.NewBuffer(bb[5:]))
}
return  err
}
func MarshalRclunkPkt (b *bytes.Buffer, t Tag, ) {
var l uint64
b.Reset()
b.Write([]byte{0,0,0,0,
uint8(Rclunk),
byte(t), byte(t>>8),
	})

{
l = uint64(b.Len())
copy(b.Bytes(), []byte{uint8(l), uint8(l>>8), uint8(l>>16), uint8(l>>24)})
}
return
}
func UnmarshalRclunkPkt (b *bytes.Buffer) ( t Tag, err error) {
var u [8]uint8
var l uint64
if _, err = b.Read(u[:2]); err != nil {
err = fmt.Errorf("pkt too short for Tag; need 2, have %d", b.Len())
return
}
l = uint64(u[0]) | uint64(u[1])<<8
t = Tag(l)

if b.Len() > 0 {
err = fmt.Errorf("Packet too long: %d bytes left over after decode", b.Len())
}
return
}
func MarshalTclunkPkt (b *bytes.Buffer, t Tag, OFID FID) {
var l uint64
b.Reset()
b.Write([]byte{0,0,0,0,
uint8(Tclunk),
byte(t), byte(t>>8),
	uint8(OFID>>0),
	uint8(OFID>>8),
	uint8(OFID>>16),
	uint8(OFID>>24),
	})

{
l = uint64(b.Len())
copy(b.Bytes(), []byte{uint8(l), uint8(l>>8), uint8(l>>16), uint8(l>>24)})
}
return
}
func UnmarshalTclunkPkt (b *bytes.Buffer) (OFID FID,  t Tag, err error) {
var u [8]uint8
var l uint64
if _, err = b.Read(u[:2]); err != nil {
err = fmt.Errorf("pkt too short for Tag; need 2, have %d", b.Len())
return
}
l = uint64(u[0]) | uint64(u[1])<<8
t = Tag(l)
	if _, err = b.Read(u[:4]); err != nil {
		err = fmt.Errorf("pkt too short for uint32: need 4, have %d", b.Len())
	return
	}
	OFID = FID(u[0])
	OFID |= FID(u[1])<<8
	OFID |= FID(u[2])<<16
	OFID |= FID(u[3])<<24

if b.Len() > 0 {
err = fmt.Errorf("Packet too long: %d bytes left over after decode", b.Len())
}
return
}
func (s *Server) SrvRclunk(b*bytes.Buffer) (err error) {
	OFID,  t, err := UnmarshalTclunkPkt(b)
	//if err != nil {
	//}
	if  err := s.NS.Rclunk(OFID); err != nil {
	MarshalRerrorPkt(b, t, fmt.Sprintf("%v", err))
} else {
	MarshalRclunkPkt(b, t, )
}
	return nil
}

func (c *Client)CallTclunk (OFID FID) ( err error) {
var b = bytes.Buffer{}
if c.Trace != nil {c.Trace("%v", Tclunk)}
t := Tag(0)
r := make (chan []byte)
if c.Trace != nil { c.Trace(":tag %v, FID %v", t, c.FID)}
MarshalTclunkPkt(&b, t, OFID)
c.FromClient <- &RPCCall{b: b.Bytes(), Reply: r}
bb := <-r
if MType(bb[4]) == Rerror {
	s, _, err := UnmarshalRerrorPkt(bytes.NewBuffer(bb[5:]))
	if err != nil {
		return  err
	}
	return  fmt.Errorf("%v", s)
} else {
	 _, err = UnmarshalRclunkPkt(bytes.NewBuffer(bb[5:]))
}
return  err
}
func MarshalRremovePkt (b *bytes.Buffer, t Tag, ) {
var l uint64
b.Reset()
b.Write([]byte{0,0,0,0,
uint8(Rremove),
byte(t), byte(t>>8),
	})

{
l = uint64(b.Len())
copy(b.Bytes(), []byte{uint8(l), uint8(l>>8), uint8(l>>16), uint8(l>>24)})
}
return
}
func UnmarshalRremovePkt (b *bytes.Buffer) ( t Tag, err error) {
var u [8]uint8
var l uint64
if _, err = b.Read(u[:2]); err != nil {
err = fmt.Errorf("pkt too short for Tag; need 2, have %d", b.Len())
return
}
l = uint64(u[0]) | uint64(u[1])<<8
t = Tag(l)

if b.Len() > 0 {
err = fmt.Errorf("Packet too long: %d bytes left over after decode", b.Len())
}
return
}
func MarshalTremovePkt (b *bytes.Buffer, t Tag, OFID FID) {
var l uint64
b.Reset()
b.Write([]byte{0,0,0,0,
uint8(Tremove),
byte(t), byte(t>>8),
	uint8(OFID>>0),
	uint8(OFID>>8),
	uint8(OFID>>16),
	uint8(OFID>>24),
	})

{
l = uint64(b.Len())
copy(b.Bytes(), []byte{uint8(l), uint8(l>>8), uint8(l>>16), uint8(l>>24)})
}
return
}
func UnmarshalTremovePkt (b *bytes.Buffer) (OFID FID,  t Tag, err error) {
var u [8]uint8
var l uint64
if _, err = b.Read(u[:2]); err != nil {
err = fmt.Errorf("pkt too short for Tag; need 2, have %d", b.Len())
return
}
l = uint64(u[0]) | uint64(u[1])<<8
t = Tag(l)
	if _, err = b.Read(u[:4]); err != nil {
		err = fmt.Errorf("pkt too short for uint32: need 4, have %d", b.Len())
	return
	}
	OFID = FID(u[0])
	OFID |= FID(u[1])<<8
	OFID |= FID(u[2])<<16
	OFID |= FID(u[3])<<24

if b.Len() > 0 {
err = fmt.Errorf("Packet too long: %d bytes left over after decode", b.Len())
}
return
}
func (s *Server) SrvRremove(b*bytes.Buffer) (err error) {
	OFID,  t, err := UnmarshalTremovePkt(b)
	//if err != nil {
	//}
	if  err := s.NS.Rremove(OFID); err != nil {
	MarshalRerrorPkt(b, t, fmt.Sprintf("%v", err))
} else {
	MarshalRremovePkt(b, t, )
}
	return nil
}

func (c *Client)CallTremove (OFID FID) ( err error) {
var b = bytes.Buffer{}
if c.Trace != nil {c.Trace("%v", Tremove)}
t := Tag(0)
r := make (chan []byte)
if c.Trace != nil { c.Trace(":tag %v, FID %v", t, c.FID)}
MarshalTremovePkt(&b, t, OFID)
c.FromClient <- &RPCCall{b: b.Bytes(), Reply: r}
bb := <-r
if MType(bb[4]) == Rerror {
	s, _, err := UnmarshalRerrorPkt(bytes.NewBuffer(bb[5:]))
	if err != nil {
		return  err
	}
	return  fmt.Errorf("%v", s)
} else {
	 _, err = UnmarshalRremovePkt(bytes.NewBuffer(bb[5:]))
}
return  err
}
func MarshalRreadPkt (b *bytes.Buffer, t Tag, Data []uint8) {
var l uint64
b.Reset()
b.Write([]byte{0,0,0,0,
uint8(Rread),
byte(t), byte(t>>8),
	uint8(len(Data)>>0),
	uint8(len(Data)>>8),
	uint8(len(Data)>>16),
	uint8(len(Data)>>24),
	})
	b.Write(Data)

{
l = uint64(b.Len())
copy(b.Bytes(), []byte{uint8(l), uint8(l>>8), uint8(l>>16), uint8(l>>24)})
}
return
}
func UnmarshalRreadPkt (b *bytes.Buffer) (Data []uint8,  t Tag, err error) {
var u [8]uint8
var l uint64
if _, err = b.Read(u[:2]); err != nil {
err = fmt.Errorf("pkt too short for Tag; need 2, have %d", b.Len())
return
}
l = uint64(u[0]) | uint64(u[1])<<8
t = Tag(l)
	if _, err = b.Read(u[:4]); err != nil {
		err = fmt.Errorf("pkt too short for uint32: need 4, have %d", b.Len())
	return
	}
	l = uint64(u[0])
	l |= uint64(u[1])<<8
	l |= uint64(u[2])<<16
	l |= uint64(u[3])<<24
	Data = b.Bytes()[:l]
	_ = b.Next(int(l))

if b.Len() > 0 {
err = fmt.Errorf("Packet too long: %d bytes left over after decode", b.Len())
}
return
}
func MarshalTreadPkt (b *bytes.Buffer, t Tag, OFID FID, Off Offset, Len Count) {
var l uint64
b.Reset()
b.Write([]byte{0,0,0,0,
uint8(Tread),
byte(t), byte(t>>8),
	uint8(OFID>>0),
	uint8(OFID>>8),
	uint8(OFID>>16),
	uint8(OFID>>24),
	uint8(Off>>0),
	uint8(Off>>8),
	uint8(Off>>16),
	uint8(Off>>24),
	uint8(Off>>32),
	uint8(Off>>40),
	uint8(Off>>48),
	uint8(Off>>56),
	uint8(Len>>0),
	uint8(Len>>8),
	uint8(Len>>16),
	uint8(Len>>24),
	})

{
l = uint64(b.Len())
copy(b.Bytes(), []byte{uint8(l), uint8(l>>8), uint8(l>>16), uint8(l>>24)})
}
return
}
func UnmarshalTreadPkt (b *bytes.Buffer) (OFID FID, Off Offset, Len Count,  t Tag, err error) {
var u [8]uint8
var l uint64
if _, err = b.Read(u[:2]); err != nil {
err = fmt.Errorf("pkt too short for Tag; need 2, have %d", b.Len())
return
}
l = uint64(u[0]) | uint64(u[1])<<8
t = Tag(l)
	if _, err = b.Read(u[:4]); err != nil {
		err = fmt.Errorf("pkt too short for uint32: need 4, have %d", b.Len())
	return
	}
	OFID = FID(u[0])
	OFID |= FID(u[1])<<8
	OFID |= FID(u[2])<<16
	OFID |= FID(u[3])<<24
	if _, err = b.Read(u[:8]); err != nil {
		err = fmt.Errorf("pkt too short for uint64: need 8, have %d", b.Len())
	return
	}
	Off = Offset(u[0])
	Off |= Offset(u[1])<<8
	Off |= Offset(u[2])<<16
	Off |= Offset(u[3])<<24
	Off |= Offset(u[4])<<32
	Off |= Offset(u[5])<<40
	Off |= Offset(u[6])<<48
	Off |= Offset(u[7])<<56
	if _, err = b.Read(u[:4]); err != nil {
		err = fmt.Errorf("pkt too short for uint32: need 4, have %d", b.Len())
	return
	}
	Len = Count(u[0])
	Len |= Count(u[1])<<8
	Len |= Count(u[2])<<16
	Len |= Count(u[3])<<24

if b.Len() > 0 {
err = fmt.Errorf("Packet too long: %d bytes left over after decode", b.Len())
}
return
}
func (s *Server) SrvRread(b*bytes.Buffer) (err error) {
	OFID, Off, Len,  t, err := UnmarshalTreadPkt(b)
	//if err != nil {
	//}
	if Data,  err := s.NS.Rread(OFID, Off, Len); err != nil {
	MarshalRerrorPkt(b, t, fmt.Sprintf("%v", err))
} else {
	MarshalRreadPkt(b, t, Data)
}
	return nil
}

func (c *Client)CallTread (OFID FID, Off Offset, Len Count) (Data []uint8,  err error) {
var b = bytes.Buffer{}
if c.Trace != nil {c.Trace("%v", Tread)}
t := Tag(0)
r := make (chan []byte)
if c.Trace != nil { c.Trace(":tag %v, FID %v", t, c.FID)}
MarshalTreadPkt(&b, t, OFID, Off, Len)
c.FromClient <- &RPCCall{b: b.Bytes(), Reply: r}
bb := <-r
if MType(bb[4]) == Rerror {
	s, _, err := UnmarshalRerrorPkt(bytes.NewBuffer(bb[5:]))
	if err != nil {
		return Data,  err
	}
	return Data,  fmt.Errorf("%v", s)
} else {
	Data,  _, err = UnmarshalRreadPkt(bytes.NewBuffer(bb[5:]))
}
return Data,  err
}
func MarshalRwritePkt (b *bytes.Buffer, t Tag, RLen Count) {
var l uint64
b.Reset()
b.Write([]byte{0,0,0,0,
uint8(Rwrite),
byte(t), byte(t>>8),
	uint8(RLen>>0),
	uint8(RLen>>8),
	uint8(RLen>>16),
	uint8(RLen>>24),
	})

{
l = uint64(b.Len())
copy(b.Bytes(), []byte{uint8(l), uint8(l>>8), uint8(l>>16), uint8(l>>24)})
}
return
}
func UnmarshalRwritePkt (b *bytes.Buffer) (RLen Count,  t Tag, err error) {
var u [8]uint8
var l uint64
if _, err = b.Read(u[:2]); err != nil {
err = fmt.Errorf("pkt too short for Tag; need 2, have %d", b.Len())
return
}
l = uint64(u[0]) | uint64(u[1])<<8
t = Tag(l)
	if _, err = b.Read(u[:4]); err != nil {
		err = fmt.Errorf("pkt too short for uint32: need 4, have %d", b.Len())
	return
	}
	RLen = Count(u[0])
	RLen |= Count(u[1])<<8
	RLen |= Count(u[2])<<16
	RLen |= Count(u[3])<<24

if b.Len() > 0 {
err = fmt.Errorf("Packet too long: %d bytes left over after decode", b.Len())
}
return
}
func MarshalTwritePkt (b *bytes.Buffer, t Tag, OFID FID, Off Offset, Data []uint8) {
var l uint64
b.Reset()
b.Write([]byte{0,0,0,0,
uint8(Twrite),
byte(t), byte(t>>8),
	uint8(OFID>>0),
	uint8(OFID>>8),
	uint8(OFID>>16),
	uint8(OFID>>24),
	uint8(Off>>0),
	uint8(Off>>8),
	uint8(Off>>16),
	uint8(Off>>24),
	uint8(Off>>32),
	uint8(Off>>40),
	uint8(Off>>48),
	uint8(Off>>56),
	uint8(len(Data)>>0),
	uint8(len(Data)>>8),
	uint8(len(Data)>>16),
	uint8(len(Data)>>24),
	})
	b.Write(Data)

{
l = uint64(b.Len())
copy(b.Bytes(), []byte{uint8(l), uint8(l>>8), uint8(l>>16), uint8(l>>24)})
}
return
}
func UnmarshalTwritePkt (b *bytes.Buffer) (OFID FID, Off Offset, Data []uint8,  t Tag, err error) {
var u [8]uint8
var l uint64
if _, err = b.Read(u[:2]); err != nil {
err = fmt.Errorf("pkt too short for Tag; need 2, have %d", b.Len())
return
}
l = uint64(u[0]) | uint64(u[1])<<8
t = Tag(l)
	if _, err = b.Read(u[:4]); err != nil {
		err = fmt.Errorf("pkt too short for uint32: need 4, have %d", b.Len())
	return
	}
	OFID = FID(u[0])
	OFID |= FID(u[1])<<8
	OFID |= FID(u[2])<<16
	OFID |= FID(u[3])<<24
	if _, err = b.Read(u[:8]); err != nil {
		err = fmt.Errorf("pkt too short for uint64: need 8, have %d", b.Len())
	return
	}
	Off = Offset(u[0])
	Off |= Offset(u[1])<<8
	Off |= Offset(u[2])<<16
	Off |= Offset(u[3])<<24
	Off |= Offset(u[4])<<32
	Off |= Offset(u[5])<<40
	Off |= Offset(u[6])<<48
	Off |= Offset(u[7])<<56
	if _, err = b.Read(u[:4]); err != nil {
		err = fmt.Errorf("pkt too short for uint32: need 4, have %d", b.Len())
	return
	}
	l = uint64(u[0])
	l |= uint64(u[1])<<8
	l |= uint64(u[2])<<16
	l |= uint64(u[3])<<24
	Data = b.Bytes()[:l]
	_ = b.Next(int(l))

if b.Len() > 0 {
err = fmt.Errorf("Packet too long: %d bytes left over after decode", b.Len())
}
return
}
func (s *Server) SrvRwrite(b*bytes.Buffer) (err error) {
	OFID, Off, Data,  t, err := UnmarshalTwritePkt(b)
	//if err != nil {
	//}
	if RLen,  err := s.NS.Rwrite(OFID, Off, Data); err != nil {
	MarshalRerrorPkt(b, t, fmt.Sprintf("%v", err))
} else {
	MarshalRwritePkt(b, t, RLen)
}
	return nil
}

func (c *Client)CallTwrite (OFID FID, Off Offset, Data []uint8) (RLen Count,  err error) {
var b = bytes.Buffer{}
if c.Trace != nil {c.Trace("%v", Twrite)}
t := Tag(0)
r := make (chan []byte)
if c.Trace != nil { c.Trace(":tag %v, FID %v", t, c.FID)}
MarshalTwritePkt(&b, t, OFID, Off, Data)
c.FromClient <- &RPCCall{b: b.Bytes(), Reply: r}
bb := <-r
if MType(bb[4]) == Rerror {
	s, _, err := UnmarshalRerrorPkt(bytes.NewBuffer(bb[5:]))
	if err != nil {
		return RLen,  err
	}
	return RLen,  fmt.Errorf("%v", s)
} else {
	RLen,  _, err = UnmarshalRwritePkt(bytes.NewBuffer(bb[5:]))
}
return RLen,  err
}
func ServerError (b *bytes.Buffer, s string) {
	var u [8]byte
	// This can't really happen. 
	if _, err := b.Read(u[:2]); err != nil {
		return
	}
	t := Tag(uint16(u[0])|uint16(u[1])<<8)
	MarshalRerrorPkt (b, t, s)
}
func Marshaldir (b *bytes.Buffer, D Dir) {
var l uint64
b.Reset()
b.Write([]byte{0,0,})
	b.Write([]byte{	uint8(D.Type>>0),
	uint8(D.Type>>8),
	uint8(D.Dev>>0),
	uint8(D.Dev>>8),
	uint8(D.Dev>>16),
	uint8(D.Dev>>24),
	uint8(D.QID.Type>>0),
	uint8(D.QID.Version>>0),
	uint8(D.QID.Version>>8),
	uint8(D.QID.Version>>16),
	uint8(D.QID.Version>>24),
	uint8(D.QID.Path>>0),
	uint8(D.QID.Path>>8),
	uint8(D.QID.Path>>16),
	uint8(D.QID.Path>>24),
	uint8(D.QID.Path>>32),
	uint8(D.QID.Path>>40),
	uint8(D.QID.Path>>48),
	uint8(D.QID.Path>>56),
	uint8(D.Mode>>0),
	uint8(D.Mode>>8),
	uint8(D.Mode>>16),
	uint8(D.Mode>>24),
	uint8(D.Atime>>0),
	uint8(D.Atime>>8),
	uint8(D.Atime>>16),
	uint8(D.Atime>>24),
	uint8(D.Mtime>>0),
	uint8(D.Mtime>>8),
	uint8(D.Mtime>>16),
	uint8(D.Mtime>>24),
	uint8(D.Length>>0),
	uint8(D.Length>>8),
	uint8(D.Length>>16),
	uint8(D.Length>>24),
	uint8(D.Length>>32),
	uint8(D.Length>>40),
	uint8(D.Length>>48),
	uint8(D.Length>>56),
	uint8(len(D.Name)),uint8(len(D.Name)>>8),
	})
	b.Write([]byte(D.Name))
	b.Write([]byte{	uint8(len(D.User)),uint8(len(D.User)>>8),
	})
	b.Write([]byte(D.User))
	b.Write([]byte{	uint8(len(D.Group)),uint8(len(D.Group)>>8),
	})
	b.Write([]byte(D.Group))
	b.Write([]byte{	uint8(len(D.ModUser)),uint8(len(D.ModUser)>>8),
	})
	b.Write([]byte(D.ModUser))

l = uint64(b.Len()) - 2
copy(b.Bytes(), []byte{uint8(l), uint8(l>>8)})
return
}
func Unmarshaldir (b *bytes.Buffer) (D Dir,  err error) {
var u [8]uint8
var l uint64
_ = b.Next(2) // eat the length too
	if _, err = b.Read(u[:2]); err != nil {
		err = fmt.Errorf("pkt too short for uint16: need 2, have %d", b.Len())
	return
	}
	D.Type = uint16(u[0])
	D.Type |= uint16(u[1])<<8
	if _, err = b.Read(u[:4]); err != nil {
		err = fmt.Errorf("pkt too short for uint32: need 4, have %d", b.Len())
	return
	}
	D.Dev = uint32(u[0])
	D.Dev |= uint32(u[1])<<8
	D.Dev |= uint32(u[2])<<16
	D.Dev |= uint32(u[3])<<24
	if _, err = b.Read(u[:1]); err != nil {
		err = fmt.Errorf("pkt too short for uint8: need 1, have %d", b.Len())
	return
	}
	D.QID.Type = uint8(u[0])
	if _, err = b.Read(u[:4]); err != nil {
		err = fmt.Errorf("pkt too short for uint32: need 4, have %d", b.Len())
	return
	}
	D.QID.Version = uint32(u[0])
	D.QID.Version |= uint32(u[1])<<8
	D.QID.Version |= uint32(u[2])<<16
	D.QID.Version |= uint32(u[3])<<24
	if _, err = b.Read(u[:8]); err != nil {
		err = fmt.Errorf("pkt too short for uint64: need 8, have %d", b.Len())
	return
	}
	D.QID.Path = uint64(u[0])
	D.QID.Path |= uint64(u[1])<<8
	D.QID.Path |= uint64(u[2])<<16
	D.QID.Path |= uint64(u[3])<<24
	D.QID.Path |= uint64(u[4])<<32
	D.QID.Path |= uint64(u[5])<<40
	D.QID.Path |= uint64(u[6])<<48
	D.QID.Path |= uint64(u[7])<<56
	if _, err = b.Read(u[:4]); err != nil {
		err = fmt.Errorf("pkt too short for uint32: need 4, have %d", b.Len())
	return
	}
	D.Mode = uint32(u[0])
	D.Mode |= uint32(u[1])<<8
	D.Mode |= uint32(u[2])<<16
	D.Mode |= uint32(u[3])<<24
	if _, err = b.Read(u[:4]); err != nil {
		err = fmt.Errorf("pkt too short for uint32: need 4, have %d", b.Len())
	return
	}
	D.Atime = uint32(u[0])
	D.Atime |= uint32(u[1])<<8
	D.Atime |= uint32(u[2])<<16
	D.Atime |= uint32(u[3])<<24
	if _, err = b.Read(u[:4]); err != nil {
		err = fmt.Errorf("pkt too short for uint32: need 4, have %d", b.Len())
	return
	}
	D.Mtime = uint32(u[0])
	D.Mtime |= uint32(u[1])<<8
	D.Mtime |= uint32(u[2])<<16
	D.Mtime |= uint32(u[3])<<24
	if _, err = b.Read(u[:8]); err != nil {
		err = fmt.Errorf("pkt too short for uint64: need 8, have %d", b.Len())
	return
	}
	D.Length = uint64(u[0])
	D.Length |= uint64(u[1])<<8
	D.Length |= uint64(u[2])<<16
	D.Length |= uint64(u[3])<<24
	D.Length |= uint64(u[4])<<32
	D.Length |= uint64(u[5])<<40
	D.Length |= uint64(u[6])<<48
	D.Length |= uint64(u[7])<<56
	if _, err = b.Read(u[:2]); err != nil {
		err = fmt.Errorf("pkt too short for uint16: need 2, have %d", b.Len())
	return
	}
	l = uint64(u[0])
	l |= uint64(u[1])<<8
	if b.Len() < int(l) {
		err = fmt.Errorf("pkt too short for string: need %d, have %d", l, b.Len())
	return
	}
	D.Name = string(b.Bytes()[:l])
	_ = b.Next(int(l))
	if _, err = b.Read(u[:2]); err != nil {
		err = fmt.Errorf("pkt too short for uint16: need 2, have %d", b.Len())
	return
	}
	l = uint64(u[0])
	l |= uint64(u[1])<<8
	if b.Len() < int(l) {
		err = fmt.Errorf("pkt too short for string: need %d, have %d", l, b.Len())
	return
	}
	D.User = string(b.Bytes()[:l])
	_ = b.Next(int(l))
	if _, err = b.Read(u[:2]); err != nil {
		err = fmt.Errorf("pkt too short for uint16: need 2, have %d", b.Len())
	return
	}
	l = uint64(u[0])
	l |= uint64(u[1])<<8
	if b.Len() < int(l) {
		err = fmt.Errorf("pkt too short for string: need %d, have %d", l, b.Len())
	return
	}
	D.Group = string(b.Bytes()[:l])
	_ = b.Next(int(l))
	if _, err = b.Read(u[:2]); err != nil {
		err = fmt.Errorf("pkt too short for uint16: need 2, have %d", b.Len())
	return
	}
	l = uint64(u[0])
	l |= uint64(u[1])<<8
	if b.Len() < int(l) {
		err = fmt.Errorf("pkt too short for string: need %d, have %d", l, b.Len())
	return
	}
	D.ModUser = string(b.Bytes()[:l])
	_ = b.Next(int(l))

return
}
