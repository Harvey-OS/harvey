package main

import (
	"bytes"
	"debug/elf"
	"fmt"
	"io/ioutil"
	"os"
	"os/exec"
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

void
rdb(void)
{
	splhi();
	iprint("rdb...not installed\n");
	for(;;);
}

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
};

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
		.attr = SG_SHARED,
		.name = "shared",
		.size = SEGMAXPG,
	},
	{
		.attr = SG_BSS,
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
		failOn(err)

		run(nil, exec.Command(tools["strip"], "-o", tmpf.Name(), path))

		in, err = ioutil.ReadAll(tmpf)
		failOn(err)

		tmpf.Close()
		os.Remove(tmpf.Name())
	} else {
		var file *os.File
		var err error

		file, err = os.Open(path)
		failOn(err)

		in, err = ioutil.ReadAll(file)
		failOn(err)

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
	for name, path := range kern.Ramfiles {
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
	failOn(tmpl.Execute(codebuf, vars))
	return codebuf.Bytes()
}
