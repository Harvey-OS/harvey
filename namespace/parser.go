// Copyright 2020 Harvey OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// Package namespace parses name space description files
// https://plan9.io/magic/man2html/6/namespace
package namespace

import (
	"bufio"
	"fmt"
	"io"
	"os"
	"strings"
)

// Parse takes a namespace file and returns a collection
// of operations.
func Parse(r io.Reader) (File, error) {
	scanner := bufio.NewScanner(r)

	cmds := []cmd{}

	for scanner.Scan() {
		buf := scanner.Bytes()
		if len(buf) <= 0 {
			continue
		}
		r := buf[0]
		// Blank lines and lines with # as the first non–space character are ignored.
		if r == '#' || r == ' ' {
			continue
		}
		cmd, err := parseLine(scanner.Text())
		if err != nil {
			return nil, err
		}
		cmds = append(cmds, cmd)
	}
	if err := scanner.Err(); err != nil {
		fmt.Fprintln(os.Stderr, "reading standard input:", err)
	}
	return cmds, nil
}

func ParseFlags(args []string) (mountflag, []string) {
	flag := REPL
	for i, arg := range args {
		// these args are passed trough strings.Fields which doesn't return empty strings
		// so this is ok.
		if arg[0] == '-' {
			var r byte
			args = append(args[:i], args[i+1:]...)
		PARSE_FLAG:
			if len(arg) > 0 {
				r, arg = arg[0], arg[1:]
				switch r {
				case 'a':
					flag |= AFTER
				case 'b':
					flag |= BEFORE
				case 'c':
					flag |= CREATE
				case 'C':
					flag |= CACHE
				default:
				}
				goto PARSE_FLAG
			}
		}
	}
	return flag, args
}

func parseLine(line string) (cmd, error) {

	var arg string
	args := strings.Fields(line)
	arg = args[0]
	args = args[1:]
	trap := syzcall(0)

	c := cmd{
		syscall: trap,
		flag:    REPL,
		args:    args,
	}
	switch arg {
	case "bind":
		c.syscall = BIND
	case "mount":
		c.syscall = MOUNT
	case "unmount":
		c.syscall = UNMOUNT
	case "clear":
		c.syscall = RFORK
	case "cd":
		c.syscall = CHDIR
	case ".":
		c.syscall = INCLUDE
	case "import":
		c.syscall = IMPORT
	default:
		panic(arg)
	}

	c.flag, c.args = ParseFlags(args)

	return c, nil
}
