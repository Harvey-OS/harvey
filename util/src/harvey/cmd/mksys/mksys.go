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
	"encoding/json"
	"flag"
	"fmt"
	"io/ioutil"
	"log"
	"os"
	"path"
	"strings"
	"text/template"
)

type Syscall struct {
	Ret     []string
	Args    []string
	Name    string
	Id      uint32
	Define  string
	Sysname string
	Libname string
	Fudge   string   `json:"-"`
	GoArgs  []string `json:"-"`
	Ret0    string   `json:"-"`
}

type Syserror struct {
	Name   string
	String string
	Id     uint32
}

type Bootmethods struct {
	Name    string
	Config  string
	Connect string
	Arg	string
}

type Sysconf struct {
	Syscalls  []Syscall
	Syserrors []Syserror
	Bootmethods []Bootmethods
}

var mode = flag.String("mode", "", "must be one of: sys.h, sysdecl.h, syscallfiles, systab.c, error.h, errstr.h, sys_harvey.s, sysnum.go")
var outpath = flag.String("o", "", "path/to/output.c")

func usage(msg string) {
	fmt.Fprintf(os.Stderr, "mksys: invoked via %v\n", os.Args)
	fmt.Fprintln(os.Stderr, msg)
	fmt.Fprint(os.Stderr, "Usage: mksys [-o outpath] -mode=MODE path/to/sysconf.json\n")
	flag.PrintDefaults()
	os.Exit(1)
}

func main() {

	flag.Parse()

	if flag.NArg() != 1 {
		usage("no path to sysconf.json")
	}

	outfile := os.Stdout
	if *mode != "syscallfiles" && *outpath != "" {
		of, err := os.Create(*outpath)
		if err != nil {
			log.Fatal(err)
		}
		outfile = of
	}

	buf, err := ioutil.ReadFile(flag.Arg(0))
	if err != nil {
		log.Fatal(err)
	}

	var sysconf Sysconf
	err = json.Unmarshal(buf, &sysconf)
	if err != nil {
		log.Fatal(err)
	}

	syscalls := sysconf.Syscalls
	syserrors := sysconf.Syserrors
	bootmethods := sysconf.Bootmethods
	for i := range syscalls {
		if syscalls[i].Define == "" {
			syscalls[i].Define = strings.ToUpper(syscalls[i].Name)
		}
		if syscalls[i].Sysname == "" {
			syscalls[i].Sysname = "sys" + syscalls[i].Name
		}
		if syscalls[i].Libname == "" {
			syscalls[i].Libname = syscalls[i].Name
		}
	}

	switch *mode {
	case "sys_harvey.s":
		a := os.Getenv("ARCH")
		switch a {
		case "aarch64":
			usage("aarch64 support is incomplete")
		case "amd64":
		case "riscv":
			usage("riscv support is incomplete")
		default:
			usage(a + " is not supported.")
		}
		syscallargs := []string{"DI", "SI", "DX", "R10", "R8", "R9"}
		//funcallregs := []string{ "DI", "SI", "DX", "CX", "R8", "R9" };
		for i := range syscalls {
			goargs := []string{}
			fpoff := 0
			for k := range syscalls[i].Args {
				switch syscalls[i].Args[k] {
				case "int32_t", "uint32_t":
					goargs = append(goargs, fmt.Sprintf("MOVL	arg%d+%d(FP), %s", k, fpoff, syscallargs[k]))
					fpoff += 4
				case "void*", "char*", "char**", "uint8_t*", "int32_t*", "uint64_t*", "int64_t*", "int64_t":
					fpoff = (fpoff + 7) & ^7
					goargs = append(goargs, fmt.Sprintf("MOVQ	arg%d+%d(FP), %s", k, fpoff, syscallargs[k]))
					fpoff += 8
				default:
					log.Fatalf("unsupported arg %s in syscall: %v", syscalls[i].Args[k], syscalls[i])
				}
			}
			syscalls[i].GoArgs = goargs
			switch syscalls[i].Ret[0] {
			case "int32_t", "uint32_t":
				syscalls[i].Ret0 = fmt.Sprintf("MOVL	AX, ret+%d(FP)", fpoff)
				fpoff += 4
			case "void*", "char*", "char**", "uint8_t*", "int32_t*", "uint64_t*", "int64_t*", "int64_t":
				fpoff = (fpoff + 7) & ^7
				syscalls[i].Ret0 = fmt.Sprintf("MOVQ	AX, ret+%d(FP)", fpoff)
				fpoff += 8
			default:
				log.Fatalf("unsupported Ret[0] in syscall: %v", syscalls[i])
			}
		}
		tmpl, err := template.New("sys_harvey.s").Parse(`/* automatically generated by mksys */
/* System calls for AMD64, Harvey */
#include "go_asm.h"
#include "go_tls.h"
#include "textflag.h"
{{ range . }}
TEXT runtime·{{ .Libname }}(SB),NOSPLIT,$0
{{ range .GoArgs }}	{{ . }}
{{ end }}	MOVQ	${{ .Id }}, AX
	SYSCALL
	{{ .Ret0 }}
	RET
{{ end }}
`)
		if err != nil {
			log.Fatal(err)
		}

		err = tmpl.Execute(outfile, syscalls)
		if err != nil {
			log.Fatal(err)
		}

	case "syscallfiles":
		var tmpl *template.Template
		var err error
		a := os.Getenv("ARCH")
		switch a {
		case "aarch64":
		tmpl, err = template.New("syscall.s").Parse(`/* automatically generated by mksys */
.globl	{{ .Libname }}
{{ .Libname }}:
	mov	x8, #{{ .Id }}
	svc	#0
	ret
`)
		case "amd64":
		tmpl, err = template.New("syscall.s").Parse(`/* automatically generated by mksys */
.globl	{{ .Libname }}
{{ .Libname }}:
	movq %rcx, %r10 /* rcx gets smashed by systenter. Use r10.*/
	movq ${{ .Id }},%rax  /* Put the system call into rax, just like linux. */
	syscall
	ret
`)
		if err != nil {
			log.Fatal(err)
		}
		case "riscv":
		tmpl, err = template.New("syscall.s").Parse(`/* automatically generated by mksys */
.globl	{{ .Libname }}
{{ .Libname }}:
	li a7, {{ .Id }}  /* Put the system call into a7 so we don't have to move a0-a6 around*/
	scall
	ret
`)
		if err != nil {
			log.Fatal(err)
		}
		default:
			usage(a + " is not supported for system call generation.")
		}

		for i := range syscalls {

			path := path.Join(*outpath, syscalls[i].Libname+".s")
			file, err := os.Create(path)
			if err != nil {
				log.Fatal(err)
			}

			err = tmpl.Execute(file, syscalls[i])
			if err != nil {
				log.Fatal(err)
			}

			err = file.Close()
			if err != nil {
				log.Fatal(err)
			}
		}
	case "sysnum.go":
		tmpl, err := template.New("sysnum.go").Parse(`// automatically generated by mksys
package syscall

const(
{{ range . }}	SYS_{{ .Define }} = {{ .Id }}
{{ end }}
)
`)
		err = tmpl.Execute(outfile, syscalls)
		if err != nil {
			log.Fatal(err)
		}

	case "sys.h":
		tmpl, err := template.New("sys.h").Parse(`/* automatically generated by mksys */
{{ range . }}#define {{ .Define }} {{ .Id }}
{{ end }}
`)
		err = tmpl.Execute(outfile, syscalls)
		if err != nil {
			log.Fatal(err)
		}

	case "sysdecl.h":
		tmpl, err := template.New("sysdecl.h").Parse(`/* automatically generated by mksys */
{{ range . }}extern {{ .Ret0 }} {{ .Libname }}({{ range $i, $e := .Args }}{{ if $i }}, {{ end }}{{ $e }}{{ end }});
{{ end }}
`)
		err = tmpl.Execute(outfile, syscalls)
		if err != nil {
			log.Fatal(err)
		}

	case "systab.c":
		for i := range syscalls {
			var fudge string
			switch syscalls[i].Ret[0] {
			case "int32_t":
				fudge = "{ .i = -1 }"
			case "int64_t":
				fudge = "{ .vl = -1ll }"
			case "void*", "char*":
				fudge = "{ .v = (void*)-1ll }"
			default:
				log.Fatalf("unsupported Ret[0] in syscall: %v", syscalls[i])
			}
			if syscalls[i].Fudge == "" {
				syscalls[i].Fudge = fudge
			}

			syscalls[i].Ret0 = syscalls[i].Ret[0]
		}
		tmpl, err := template.New("systab.c").Parse(`/* automatically generated by mksys */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "../../libc/9syscall/sys.h"

{{ range . }}extern void {{ .Sysname }}(Ar0*, ...);
{{ end }}
Systab systab[] = {
{{ range . }}[{{ .Define }}] = { "{{ .Name }}", {{ .Sysname }}, {{ .Fudge }} },
{{ end }}
};
int nsyscall = nelem(systab);
`)
		err = tmpl.Execute(outfile, syscalls)
		if err != nil {
			log.Fatal(err)
		}

	case "error.h":
		tmpl, err := template.New("error.h").Parse(`/* automatically generated by mksys */
{{ range . }}extern char {{ .Name }}[]; /* {{ .String }} */
{{ end }}
`)
		err = tmpl.Execute(outfile, syserrors)
		if err != nil {
			log.Fatal(err)
		}

	case "errstr.h":
		tmpl, err := template.New("errstr.h").Parse(`/* automatically generated by mksys */
{{ range . }}char {{ .Name }}[] = "{{ .String }}";
{{ end }}
`)
		err = tmpl.Execute(outfile, syserrors)
		if err != nil {
			log.Fatal(err)
		}
	case "bootcpu.c":
		tmpl, err := template.New("bootcpu.c").Parse(`/* automatically generated by mksys */
#include <u.h>
#include <libc.h>

#include "../boot/boot.h"

Method method[] = {
{{ range . }}{ "{{.Name}}", {{.Config}}, {{.Connect}}, "{{.Arg}}", },
{{ end }}
	{ nil },
};

int cpuflag = 1;
char* rootdir = "/root";
char* bootdisk = "#S/sdE0/";
extern void boot(int, char**);

void
main(int argc, char **argv)
{
		boot(argc, argv);
}
int (*cfs)(int) = 0;
`)
		err = tmpl.Execute(outfile, bootmethods)
		if err != nil {
			log.Fatal(err)
		}
	}
}
