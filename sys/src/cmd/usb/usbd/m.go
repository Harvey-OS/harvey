/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

package main

import (
	"encoding/json"
	"flag"
	"fmt"
	"io/ioutil"
	"log"
	"os"
	"text/template"
)

type Embed struct {
	Name string
	Class string
	CSP [4]string
	VID string
	DID string
}

type SysConf struct {
	Embeds  []Embed
}

var outpath = flag.String("o", "", "path/to/output.c")

func usage(msg string) {
	fmt.Fprint(os.Stderr, msg)
	fmt.Fprint(os.Stderr, "Usage: usb [-o outpath] path/to/usbdb.json\n")
	flag.PrintDefaults()
	os.Exit(1)
}

func main() {

	flag.Parse()

	if flag.NArg() != 1 {
		usage("no path to sysconf.json")
	}

	outfile := os.Stdout
	if *outpath != "" {
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

	var sysconf SysConf
	err = json.Unmarshal(buf, &sysconf)
	if err != nil {
		log.Fatal(err)
	}

		tmpl, err := template.New("usbdb.c").Parse(`
/* machine generated. do not edit */
#include <u.h>
#include <libc.h>
#include <thread.h>
#include <usb/usb.h>
#include "usbd.h"

{{ range . }}	int {{ .Name }}main(Dev*, int, char**);
{{ end }}

Devtab devtab[] = {
	/* device, entrypoint, {csp, csp, csp csp}, vid, did */
{{ range . }}	{ "{{ .Name}}", {{ .Name }}main,  { {{ .Class}} | {{ index .CSP 0}},{{index .CSP 1}},{{ index .CSP 2}}, {{ index .CSP 3}}  }, {{ .VID}}, {{.DID}}, ""},
{{ end }}
	{nil, nil,	{0, 0, 0, 0, }, -1, -1, nil},
};

/* end of machine generated */

`)
	err = tmpl.Execute(outfile, sysconf.Embeds)
	if err != nil {
		log.Fatal(err)
	}

}
