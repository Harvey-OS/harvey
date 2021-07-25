// Copyright 2015 The Ninep Authors.  All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// +build ignore

// gen is an rpc generator for the Plan 9 style XDR. It uses the types and structs
// defined in types. go. A core function, gen, creates the needed lists of
// parameters, code, and variable list for calling a Marshall function; and the
// return declaration, code, and return value list for an unmarshall function.
// You can think of an RPC as a pipline:
// marshal(parms) -> b[]byte over a network -> unmarshal -> dispatch -> reply(parms) -> unmarshal
// Since we have T messages and R messages in 9p, we adopt the following naming convention for, e.g., Version:
// MarshalTPktVersion
// UnmarshalTpktVersion
// MarshalRPktVersion
// UnmarshalRPktVersion
//
// A caller uses the MarshalT* and UnmarshallR* information. A dispatcher
// uses the  UnmarshalT* and MarshalR* information.
// Hence the caller needs the call MarshalT params, and UnmarshalR* returns;
// a dispatcher needs the UnmarshalT returns, and the MarshalR params.
package main

import (
	"bytes"
	"flag"
	"fmt"
	"io"
	"io/ioutil"
	"log"
	"reflect"
	"text/template"

	"harvey-os.org/pkg/ninep/protocol"
)

const (
	header = `
package protocol
import (
"bytes"
"fmt"
_ "log"
)
`
)

type emitter struct {
	Name  string
	MFunc string
	// Encoders always return []byte
	MParms *bytes.Buffer
	MList  *bytes.Buffer
	MLsep  string
	MCode  *bytes.Buffer

	// Decoders always take []byte as parms.
	UFunc    string
	UList    *bytes.Buffer
	UCode    *bytes.Buffer
	URet     *bytes.Buffer
	inBWrite bool
}

type call struct {
	T *emitter
	R *emitter
}

type pack struct {
	n  string
	t  interface{}
	tn string
	r  interface{}
	rn string
}

const (
	serverError = `func ServerError (b *bytes.Buffer, s string) {
	var u [8]byte
	// This can't really happen. 
	if _, err := b.Read(u[:2]); err != nil {
		return
	}
	t := Tag(uint16(u[0])|uint16(u[1])<<8)
	MarshalRerrorPkt (b, t, s)
}
`
)

var (
	doDebug  = flag.Bool("d", false, "Debug prints")
	debug    = nodebug //log.Printf
	packages = []*pack{
		{n: "error", t: protocol.RerrorPkt{}, tn: "Rerror", r: protocol.RerrorPkt{}, rn: "Rerror"},
		{n: "version", t: protocol.TversionPkt{}, tn: "Tversion", r: protocol.RversionPkt{}, rn: "Rversion"},
		{n: "attach", t: protocol.TattachPkt{}, tn: "Tattach", r: protocol.RattachPkt{}, rn: "Rattach"},
		{n: "flush", t: protocol.TflushPkt{}, tn: "Tflush", r: protocol.RflushPkt{}, rn: "Rflush"},
		{n: "walk", t: protocol.TwalkPkt{}, tn: "Twalk", r: protocol.RwalkPkt{}, rn: "Rwalk"},
		{n: "open", t: protocol.TopenPkt{}, tn: "Topen", r: protocol.RopenPkt{}, rn: "Ropen"},
		{n: "create", t: protocol.TcreatePkt{}, tn: "Tcreate", r: protocol.RcreatePkt{}, rn: "Rcreate"},
		{n: "stat", t: protocol.TstatPkt{}, tn: "Tstat", r: protocol.RstatPkt{}, rn: "Rstat"},
		{n: "wstat", t: protocol.TwstatPkt{}, tn: "Twstat", r: protocol.RwstatPkt{}, rn: "Rwstat"},
		{n: "clunk", t: protocol.TclunkPkt{}, tn: "Tclunk", r: protocol.RclunkPkt{}, rn: "Rclunk"},
		{n: "remove", t: protocol.TremovePkt{}, tn: "Tremove", r: protocol.RremovePkt{}, rn: "Rremove"},
		{n: "read", t: protocol.TreadPkt{}, tn: "Tread", r: protocol.RreadPkt{}, rn: "Rread"},
		{n: "write", t: protocol.TwritePkt{}, tn: "Twrite", r: protocol.RwritePkt{}, rn: "Rwrite"},
	}
	msfunc = template.Must(template.New("ms").Parse(`func Marshal{{.MFunc}} (b *bytes.Buffer, {{.MParms}}) {
var l uint64
b.Reset()
b.Write([]byte{0,0,})
{{.MCode}}
l = uint64(b.Len()) - 2
copy(b.Bytes(), []byte{uint8(l), uint8(l>>8)})
return
}
`))
	usfunc = template.Must(template.New("us").Parse(`func Unmarshal{{.UFunc}} (b *bytes.Buffer) ({{.URet}} err error) {
var u [8]uint8
var l uint64
_ = b.Next(2) // eat the length too
{{.UCode}}
return
}
`))

	mfunc = template.Must(template.New("mt").Parse(`func Marshal{{.MFunc}}Pkt (b *bytes.Buffer, t Tag, {{.MParms}}) {
var l uint64
b.Reset()
b.Write([]byte{0,0,0,0,
uint8({{.Name}}),
byte(t), byte(t>>8),
{{.MCode}}
{
l = uint64(b.Len())
copy(b.Bytes(), []byte{uint8(l), uint8(l>>8), uint8(l>>16), uint8(l>>24)})
}
return
}
`))
	ufunc = template.Must(template.New("mr").Parse(`func Unmarshal{{.UFunc}}Pkt (b *bytes.Buffer) ({{.URet}} t Tag, err error) {
var u [8]uint8
var l uint64
if _, err = b.Read(u[:2]); err != nil {
err = fmt.Errorf("pkt too short for Tag; need 2, have %d", b.Len())
return
}
l = uint64(u[0]) | uint64(u[1])<<8
t = Tag(l)
{{.UCode}}
if b.Len() > 0 {
err = fmt.Errorf("Packet too long: %d bytes left over after decode", b.Len())
}
return
}
`))
	sfunc = template.Must(template.New("s").Parse(`func (s *Server) Srv{{.R.UFunc}}(b*bytes.Buffer) (err error) {
	{{.T.MList}}{{.T.MLsep}} t, err := Unmarshal{{.T.MFunc}}Pkt(b)
	//if err != nil {
	//}
	if {{.R.MList}}{{.R.MLsep}} err := s.NS.{{.R.MFunc}}({{.T.MList}}); err != nil {
	MarshalRerrorPkt(b, t, fmt.Sprintf("%v", err))
} else {
	Marshal{{.R.MFunc}}Pkt(b, t, {{.R.MList}})
}
	return nil
}
`))
	cfunc = template.Must(template.New("s").Parse(`
func (c *Client)Call{{.T.MFunc}} ({{.T.MParms}}) ({{.R.URet}} err error) {
var b = bytes.Buffer{}
if c.Trace != nil {c.Trace("%v", {{.T.MFunc}})}
t := Tag(0)
r := make (chan []byte)
if c.Trace != nil { c.Trace(":tag %v, FID %v", t, c.FID)}
Marshal{{.T.MFunc}}Pkt(&b, t, {{.T.MList}})
c.FromClient <- &RPCCall{b: b.Bytes(), Reply: r}
bb := <-r
if MType(bb[4]) == Rerror {
	s, _, err := UnmarshalRerrorPkt(bytes.NewBuffer(bb[5:]))
	if err != nil {
		return {{.R.UList}} err
	}
	return {{.R.UList}} fmt.Errorf("%v", s)
} else {
	{{.R.MList}}{{.R.MLsep}} _, err = Unmarshal{{.R.UFunc}}Pkt(bytes.NewBuffer(bb[5:]))
}
return {{.R.UList}} err
}
`))
)

func nodebug(string, ...interface{}) {
}

func newCall(p *pack) *call {
	c := &call{}
	// We set inBWrite to true because the prologue marshal code sets up some default writes to b
	c.T = &emitter{"T" + p.n, p.tn, &bytes.Buffer{}, &bytes.Buffer{}, "", &bytes.Buffer{}, p.tn, &bytes.Buffer{}, &bytes.Buffer{}, &bytes.Buffer{}, true}
	c.R = &emitter{"R" + p.n, p.rn, &bytes.Buffer{}, &bytes.Buffer{}, "", &bytes.Buffer{}, p.rn, &bytes.Buffer{}, &bytes.Buffer{}, &bytes.Buffer{}, true}
	return c
}

func emitEncodeInt(v interface{}, n string, l int, e *emitter) {
	debug("emit %v, %v", n, l)
	for i := 0; i < l; i++ {
		if !e.inBWrite {
			e.MCode.WriteString("\tb.Write([]byte{")
			e.inBWrite = true
		}
		e.MCode.WriteString(fmt.Sprintf("\tuint8(%v>>%v),\n", n, i*8))
	}
}

func emitDecodeInt(v interface{}, n string, l int, e *emitter) {
	t := reflect.ValueOf(v).Type().Name()
	debug("emit reflect.ValueOf(v) %s %v, %v", t, n, l)
	e.UCode.WriteString(fmt.Sprintf("\tif _, err = b.Read(u[:%v]); err != nil {\n\t\terr = fmt.Errorf(\"pkt too short for uint%v: need %v, have %%d\", b.Len())\n\treturn\n\t}\n", l, l*8, l))
	e.UCode.WriteString(fmt.Sprintf("\t%v = %s(u[0])\n", n, t))
	for i := 1; i < l; i++ {
		e.UCode.WriteString(fmt.Sprintf("\t%v |= %s(u[%d])<<%v\n", n, t, i, i*8))
	}
}

// TODO: templates.
func emitEncodeString(v interface{}, n string, e *emitter) {
	if !e.inBWrite {
		e.MCode.WriteString("\tb.Write([]byte{")
		e.inBWrite = true
	}
	e.MCode.WriteString(fmt.Sprintf("\tuint8(len(%v)),uint8(len(%v)>>8),\n", n, n))
	e.MCode.WriteString("\t})\n")
	e.inBWrite = false
	e.MCode.WriteString(fmt.Sprintf("\tb.Write([]byte(%v))\n", n))
}

func emitDecodeString(n string, e *emitter) {
	var l uint64
	emitDecodeInt(l, "l", 2, e)
	e.UCode.WriteString(fmt.Sprintf("\tif b.Len() < int(l) {\n\t\terr = fmt.Errorf(\"pkt too short for string: need %%d, have %%d\", l, b.Len())\n\treturn\n\t}\n"))
	e.UCode.WriteString(fmt.Sprintf("\t%v = string(b.Bytes()[:l])\n", n))
	e.UCode.WriteString("\t_ = b.Next(int(l))\n")
}

func genEncodeStruct(v interface{}, n string, e *emitter) error {
	debug("genEncodeStruct(%T, %v, %v)", v, n, e)
	t := reflect.ValueOf(v)
	for i := 0; i < t.NumField(); i++ {
		f := t.Field(i)
		fn := t.Type().Field(i).Name
		debug("genEncodeStruct %T n %v field %d %v %v\n", t, n, i, f.Type(), f.Type().Name())
		genEncodeData(f.Interface(), n+fn, e)
	}
	return nil
}

func genDecodeStruct(v interface{}, n string, e *emitter) error {
	debug("genDecodeStruct(%T, %v, %v)", v, n, "")
	t := reflect.ValueOf(v)
	for i := 0; i < t.NumField(); i++ {
		f := t.Field(i)
		fn := t.Type().Field(i).Name
		debug("genDecodeStruct %T n %v field %d %v %v\n", t, n, i, f.Type(), f.Type().Name())
		genDecodeData(f.Interface(), n+fn, e)
	}
	return nil
}

// TODO: there has to be a smarter way to do slices.
func genEncodeSlice(v interface{}, n string, e *emitter) error {
	t := fmt.Sprintf("%T", v)
	switch t {
	case "[]string":
		var u uint16
		emitEncodeInt(u, fmt.Sprintf("len(%v)", n), 2, e)
		if e.inBWrite {
			e.MCode.WriteString("\t})\n")
			e.inBWrite = false
		}
		e.MCode.WriteString(fmt.Sprintf("for i := range %v {\n", n))
		var s string
		genEncodeData(s, n+"[i]", e)
		e.MCode.WriteString("}\n")
	case "[]protocol.QID":
		var u uint16
		emitEncodeInt(u, fmt.Sprintf("len(%v)", n), 2, e)
		if e.inBWrite {
			e.MCode.WriteString("\t})\n")
			e.inBWrite = false
		}
		e.MCode.WriteString(fmt.Sprintf("for i := range %v {\n", n))
		genEncodeData(protocol.QID{}, n+"[i]", e)
		e.MCode.WriteString("\t})\n")
		e.inBWrite = false
		e.MCode.WriteString("}\n")
	case "[]byte", "[]uint8":
		var u uint32
		emitEncodeInt(u, fmt.Sprintf("len(%v)", n), 4, e)
		if e.inBWrite {
			e.MCode.WriteString("\t})\n")
			e.inBWrite = false
		}
		e.MCode.WriteString(fmt.Sprintf("\tb.Write(%v)\n", n))
	case "[]protocol.DataCnt16":
		var u uint16
		emitEncodeInt(u, fmt.Sprintf("len(%v)", n), 2, e)
		if e.inBWrite {
			e.MCode.WriteString("\t})\n")
			e.inBWrite = false
		}
		e.MCode.WriteString(fmt.Sprintf("\tb.Write(%v)\n", n))
	default:
		log.Printf("genEncodeSlice: Can't handle slice of %s", t)
	}
	return nil
}

func genDecodeSlice(v interface{}, n string, e *emitter) error {
	// Sadly, []byte is not encoded like []everything else.
	t := fmt.Sprintf("%T", v)
	switch t {
	case "[]string":
		var u uint64
		emitDecodeInt(u, "l", 2, e)
		e.UCode.WriteString(fmt.Sprintf("\t%v = make([]string, l)\n", n))
		e.UCode.WriteString(fmt.Sprintf("for i := range %v {\n", n))
		var s string
		genDecodeData(s, n+"[i]", e)
		e.UCode.WriteString("}\n")
	case "[]protocol.QID":
		var u uint64
		emitDecodeInt(u, "l", 2, e)
		e.UCode.WriteString(fmt.Sprintf("\t%v = make([]QID, l)\n", n))
		e.UCode.WriteString(fmt.Sprintf("for i := range %v {\n", n))
		genDecodeData(protocol.QID{}, n+"[i]", e)
		e.UCode.WriteString("}\n")
	case "[]byte", "[]uint8":
		var u uint64
		emitDecodeInt(u, "l", 4, e)
		e.UCode.WriteString(fmt.Sprintf("\t%v = b.Bytes()[:l]\n", n))
		e.UCode.WriteString("\t_ = b.Next(int(l))\n")
	case "[]protocol.DataCnt16":
		var u uint64
		emitDecodeInt(u, "l", 2, e)
		e.UCode.WriteString(fmt.Sprintf("\t%v = b.Bytes()[:l]\n", n))
		e.UCode.WriteString("\t_ = b.Next(int(l))\n")
	default:
		log.Printf("genDecodeSlice: Can't handle slice of %v", t)
	}
	return nil
}

func genEncodeData(v interface{}, n string, e *emitter) error {
	debug("genEncodeData(%T, %v, %v)", v, n, e)
	s := reflect.ValueOf(v).Kind()
	switch s {
	case reflect.Uint8:
		emitEncodeInt(v, n, 1, e)
	case reflect.Uint16:
		emitEncodeInt(v, n, 2, e)
	case reflect.Uint32, reflect.Int32:
		emitEncodeInt(v, n, 4, e)
	case reflect.Uint64:
		emitEncodeInt(v, n, 8, e)
	case reflect.String:
		emitEncodeString(v, n, e)
	case reflect.Struct:
		if n != "" {
			n = n + "."
		}
		return genEncodeStruct(v, n, e)
	case reflect.Slice:
		return genEncodeSlice(v, n, e)
	default:
		log.Printf("genEncodeData: Can't handle type %T", v)
	}
	return nil
}

func genDecodeData(v interface{}, n string, e *emitter) error {
	debug("genEncodeData(%T, %v, %v)", v, n, "") //e)
	s := reflect.ValueOf(v).Kind()
	switch s {
	case reflect.Uint8:
		emitDecodeInt(v, n, 1, e)
	case reflect.Uint16:
		emitDecodeInt(v, n, 2, e)
	case reflect.Uint32, reflect.Int32:
		emitDecodeInt(v, n, 4, e)
	case reflect.Uint64:
		emitDecodeInt(v, n, 8, e)
	case reflect.String:
		emitDecodeString(n, e)
	case reflect.Struct:
		if n != "" {
			n = n + "."
		}
		debug("----------> call gendecodstruct(%v, %v, e)", v, n)
		return genDecodeStruct(v, n, e)
	case reflect.Slice:
		return genDecodeSlice(v, n, e)
	default:
		log.Printf("genDecodeData: Can't handle type %T", v)
	}
	return nil
}

// Well, it's odd, but Name sometimes comes back empty.
// I don't know why.
func tn(f reflect.Value) string {
	n := f.Type().Name()
	if n == "" {
		t := f.Type().String()
		switch t {
		case "[]protocol.QID":
			n = "[]QID"
		case "[]protocol.DataCnt16":
			n = "[]byte"
		default:
			n = t
		}
	}
	return n
}

// genParms writes the parameters for declarations (name and type)
// a list of names (for calling the encoder)
func genParms(v interface{}, n string, e *emitter) error {
	t := reflect.ValueOf(v)
	for i := 0; i < t.NumField(); i++ {
		f := t.Field(i)
		fn := t.Type().Field(i).Name
		e.MList.WriteString(e.MLsep + fn)
		e.MParms.WriteString(e.MLsep + fn + " " + tn(f))
		e.MLsep = ", "
	}
	return nil
}

// genRets writes the rets for declarations (name and type)
// a list of names
func genRets(v interface{}, n string, e *emitter) error {
	t := reflect.ValueOf(v)
	for i := 0; i < t.NumField(); i++ {
		f := t.Field(i)
		fn := t.Type().Field(i).Name
		e.UList.WriteString(fn + ", ")
		e.URet.WriteString(fn + " " + tn(f) + ", ")
	}
	return nil
}

// genMsgRPC generates the call and reply declarations and marshalers. We don't think of encoders as too separate
// because the 9p encoding is so simple.
func genMsgRPC(b io.Writer, p *pack) (*call, error) {

	c := newCall(p)

	if err := genEncodeStruct(p.r, "", c.R); err != nil {
		log.Fatalf("%v", err)
	}
	if c.R.inBWrite {
		c.R.MCode.WriteString("\t})\n")
		c.R.inBWrite = false
	}
	if err := genDecodeStruct(p.r, "", c.R); err != nil {
		log.Fatalf("%v", err)
	}

	if err := genEncodeStruct(p.t, "", c.T); err != nil {
		log.Fatalf("%v", err)
	}
	if c.T.inBWrite {
		c.T.MCode.WriteString("\t})\n")
		c.T.inBWrite = false
	}

	if err := genDecodeStruct(p.t, "", c.T); err != nil {
		log.Fatalf("%v", err)
	}

	if err := genParms(p.t, p.tn, c.T); err != nil {
		log.Fatalf("%v", err)
	}

	if err := genRets(p.t, p.tn, c.T); err != nil {
		log.Fatalf("%v", err)
	}

	if err := genParms(p.r, p.rn, c.R); err != nil {
		log.Fatalf("%v", err)
	}

	if err := genRets(p.r, p.rn, c.R); err != nil {
		log.Fatalf("%v", err)
	}

	//log.Print("e %v d %v", c.T, c.R)

	//	log.Print("------------------", c.T.MParms, "0", c.T.MList, "1", c.R.URet, "2", c.R.UList)
	//	log.Print("------------------", c.T.MCode)
	mfunc.Execute(b, c.R)
	ufunc.Execute(b, c.R)

	if p.n == "error" {
		return c, nil
	}

	mfunc.Execute(b, c.T)
	ufunc.Execute(b, c.T)
	sfunc.Execute(b, c)
	cfunc.Execute(b, c)
	return nil, nil

}

func main() {
	flag.Parse()
	if *doDebug {
		debug = log.Printf
	}
	var b = bytes.NewBufferString(header)
	for _, p := range packages {
		_, err := genMsgRPC(b, p)
		if err != nil {
			log.Fatalf("%v", err)
		}
	}
	b.WriteString(serverError)

	// yeah, it's a hack.
	dir := &emitter{"dir", "dir", &bytes.Buffer{}, &bytes.Buffer{}, "", &bytes.Buffer{}, "dir", &bytes.Buffer{}, &bytes.Buffer{}, &bytes.Buffer{}, false}
	if err := genEncodeStruct(protocol.DirPkt{}, "", dir); err != nil {
		log.Fatalf("%v", err)
	}
	if err := genDecodeStruct(protocol.DirPkt{}, "", dir); err != nil {
		log.Fatalf("%v", err)
	}
	if err := genParms(protocol.DirPkt{}, "dir", dir); err != nil {
		log.Fatalf("%v", err)
	}

	if err := genRets(protocol.DirPkt{}, "dir", dir); err != nil {
		log.Fatalf("%v", err)
	}

	msfunc.Execute(b, dir)
	usfunc.Execute(b, dir)

	if err := ioutil.WriteFile("genout.go", b.Bytes(), 0600); err != nil {
		log.Fatalf("%v", err)
	}

}
