/*
Copyright 2018 Harvey OS Team

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

package main

import (
	"bytes"
	"debug/elf"
	"fmt"
	"io/ioutil"
	"os"
	"os/exec"
	"sort"
	"text/template"
)

const kernconfTmpl = `
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "../port/error.h"
#include "io.h"

{{ range .Rootcodes }}
{{ . }}
{{ end }}

{{ range .Config.Dev }}extern Dev {{ . }}devtab;
{{ end }}
Dev *devtab[] = {
{{ range .Config.Dev }}
	&{{ . }}devtab,
{{ end }}
	nil,
};

{{ range .Config.Link }}extern void {{ . }}link(void);
{{ end }}
void
links(void)
{
{{ range .Rootnames }}addbootfile("{{ . }}", ramfs_{{ . }}_code, ramfs_{{ . }}_len);
{{ end }}
{{ range .Config.Link }}{{ . }}link();
{{ end }}
}

#include "../ip/ip.h"
{{ range .Config.Ip }}extern void {{ . }}init(Fs*);
{{ end }}
void (*ipprotoinit[])(Fs*) = {
{{ range .Config.Ip }}	{{ . }}init,
{{ end }}
	nil,
};

#include "../port/sd.h"
{{ range .Config.Sd }}extern SDifc {{ . }}ifc;
{{ end }}
SDifc* sdifc[] = {
{{ range .Config.Sd }}	&{{ . }}ifc,
{{ end }}
	nil,
};

{{ range .Config.Uart }}extern PhysUart {{ . }}physuart;
{{ end }}
PhysUart* physuart[] = {
{{ range .Config.Uart }}	&{{ . }}physuart,
{{ end }}
	nil,
};`

const vgaconfTmpl = `
#define	Image	IMAGE
#include <draw.h>
#include <memdraw.h>
#include <cursor.h>
#include "screen.h"
{{ range .Config.VGA }}extern VGAdev {{ . }}dev;
{{ end }}
VGAdev* vgadev[] = {
{{ range .Config.VGA }}	&{{ . }}dev,
{{ end }}
	nil,
};

{{ range .Config.VGA }}extern VGAcur {{ . }}cur;
{{ end }}
VGAcur* vgacur[] = {
{{ range .Config.VGA }}	&{{ . }}cur,
{{ end }}
	nil,
};

Physseg physseg[8] = {
	{
		.attr = SG_SHARED|SG_READ|SG_WRITE,
		.name = "shared",
		.size = SEGMAXPG,
	},
	{
		.attr = SG_BSS|SG_READ|SG_WRITE,
		.name = "memory",
		.size = SEGMAXPG,
	},
};
int nphysseg = 8;

{{ range .Config.Code }}{{ . }}
{{ end }}

char* conffile = "{{ .Path }}";

`

// These are the two big code generation functions.

// data2c takes the file at path and creates a C byte array containing it.
func data2c(name string, path string) string {
	var out []byte
	var in []byte

	if elf, err := elf.Open(path); err == nil {
		elf.Close()
		cwd, err := os.Getwd()
		tmpf, err := ioutil.TempFile(cwd, name)
		fail(err)

		run(nil, *shellhack, exec.Command(tools["strip"], "-o", tmpf.Name(), path))

		in, err = ioutil.ReadAll(tmpf)
		fail(err)

		tmpf.Close()
		os.Remove(tmpf.Name())
	} else {
		var file *os.File
		var err error

		file, err = os.Open(path)
		fail(err)

		in, err = ioutil.ReadAll(file)
		fail(err)

		file.Close()
	}

	total := len(in)

	out = []byte(fmt.Sprintf("static unsigned char ramfs_%s_code[] = {\n", name))
	for len(in) > 0 {
		for j := 0; j < 16 && len(in) > 0; j++ {
			out = append(out, []byte(fmt.Sprintf("0x%02x, ", in[0]))...)
			in = in[1:]
		}
		out = append(out, '\n')
	}

	out = append(out, []byte(fmt.Sprintf("0,\n};\nint ramfs_%s_len = %v;\n", name, total))...)

	return string(out)
}

// confcode creates a kernel configuration header.
func confcode(path string, kern *kernel) []byte {
	var rootcodes []string
	var rootnames []string

	// Sort keys so we can guaranteed order of iteration over ramfiles
	var ramfilenames []string
	for name := range kern.Ramfiles {
		ramfilenames = append(ramfilenames, name)
	}
	sort.Strings(ramfilenames)

	for _, name := range ramfilenames {
		path := kern.Ramfiles[name]
		code := data2c(name, fromRoot(path))
		rootcodes = append(rootcodes, code)
		rootnames = append(rootnames, name)
	}

	vars := struct {
		Path      string
		Config    kernconfig
		Rootnames []string
		Rootcodes []string
	}{
		path,
		kern.Config,
		rootnames,
		rootcodes,
	}
	tmpl := template.Must(template.New("kernconf").Parse(kernconfTmpl))
	codebuf := &bytes.Buffer{}
	fail(tmpl.Execute(codebuf, vars))
	fmt.Printf("VGA IS %v len %d\n", vars.Config.VGA, len(vars.Config.VGA))
	if len(vars.Config.VGA) > 0 {
		tmpl = template.Must(template.New("vgaconf").Parse(vgaconfTmpl))
		vgabuf := &bytes.Buffer{}
		fail(tmpl.Execute(codebuf, vars))
		codebuf.Write(vgabuf.Bytes())
	}
	return codebuf.Bytes()
}
