// Run commands received from stdin in qemu
//
// -p prompt	=> prompt to expect (default "10.0.2.15#")
// -i image		=> image to load (default $HARVEY/sys/src/9/k10/9k8cpu.32bit)
//
// ENVIRONMENT
// Needed: HARVEY
package main

import (
	"bufio"
	"fmt"
	"flag"
	"os"
	"os/exec"
	"strings"
	"github.com/kr/pty"
	"golang.org/x/crypto/ssh/terminal"
)

func main() {
	var image, prompt string
	flag.StringVar(&prompt, "p", "10.0.2.15#", "the prompt to expect")
	flag.StringVar(&image, "i", "$HARVEY/sys/src/9/k10/9k8cpu.32bit", "image to load")
	flag.Parse()
	harvey := os.Getenv("HARVEY")
	if harvey == "" {
		fmt.Printf("usage: cat cmds.rc | runqemu [-p prompt] [-i image]\n")
		fmt.Printf("error: missing $HARVEY\n");
		os.Exit(1)
	}
	image = os.ExpandEnv(image);
	if _, err := os.Stat(image); os.IsNotExist(err) {
		fmt.Printf("usage: cat cmds.rc | runqemu [-p prompt] [-i image]\n")
		fmt.Printf("error: image %s not found.\n", image)
		os.Exit(1)
	}
	if terminal.IsTerminal(0) {
		fmt.Printf("usage: cat cmds.rc | runqemu [-p prompt] [-i image]\n")
		fmt.Printf("error: runqemu is intended for automation, pipe commands in.\n")
		os.Exit(1)
	}
	
	qemuCmd := "cd $HARVEY/sys/src/9/k10 && sh $HARVEY/util/REGRESS\n"
	qemuCmd = os.ExpandEnv(qemuCmd)

	sh := exec.Command("/bin/sh")
	qemu, err := pty.Start(sh)
	if err != nil {
		fmt.Printf("REGRESS start (%s): %s", qemuCmd, err)
		os.Exit(2)
	}
	qemu.WriteString(qemuCmd)
	defer qemu.Close()
	
	exitStatus := 0
	
	qemuInput  := make(chan string)
	qemuOutputRaw := make(chan string)
	wait := make(chan int)

	scanner := bufio.NewScanner(os.Stdin)

	go func() {
		qemuOut := make([]byte, 256)
		for {
			r, err := qemu.Read(qemuOut)
			if err != nil {
				fmt.Fprintln(os.Stderr, "error:", err)
				wait <- 3
			}
			qemuOutputRaw <- string(qemuOut[0:r])
		}
	}()
	go func(){
		line := ""
		for {
			s := <- qemuOutputRaw
			line += s
			if strings.Contains(line, prompt) {
				if scanner.Scan() {
					cmd	:= scanner.Text()
					qemuInput <- fmt.Sprintf("%s\n", cmd)
				} else {
					if err := scanner.Err(); err != nil {
						fmt.Fprintln(os.Stderr, "error:", err)
						wait <- 4
				    } else {
						wait <- exitStatus
					}
					return
				}
				fmt.Printf("%s", line)
				line = ""
			} else if strings.ContainsAny(line, "\r\n") {
				if strings.Contains(line, "FAIL") {
					exitStatus = 5
				}
				fmt.Printf("%s", line)
				line = ""
			}
		}
	}()
	go func(){
		for {
			s := <- qemuInput
			i := 0;
			for {
				n, err := qemu.WriteString(s[i:])
				if err != nil {
					fmt.Fprintln(os.Stderr, "error:", err)
					wait <- 6
				}
				i += n
				if i == len(s) {
					break
				}
			}
		}
	}()

	e := <- wait
	if e == 0 {
		fmt.Printf("\nDone.\n")
	}
	os.Exit(e)
}
