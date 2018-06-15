package main

import (
	"bytes"
	"io"
	"io/ioutil"
	"log"
	"os"
	"os/exec"
	"path/filepath"
)

var (
	setupcmds = []struct {
		*exec.Cmd
		in io.Reader
	}{
		{Cmd: exec.Command("qemu-img", "create", "-f", "qcow2", "harvey.qcow2", "3G")},
		{Cmd: exec.Command("modprobe", "nbd", "max_part=63")},
		{Cmd: exec.Command("qemu-nbd", "-c", "/dev/nbd0", "harvey.qcow2")},
		{Cmd: exec.Command("dd", "conv=notrunc", "bs=440", "count=1", "if=img/syslinux-bios/mbr.bin", "of=/dev/nbd0")},
		{Cmd: exec.Command("sfdisk", "/dev/nbd0"), in: bytes.NewBufferString(part)},
		{Cmd: exec.Command("mkfs.ext4", "/dev/nbd0p1")},
		{Cmd: exec.Command("mount", "/dev/nbd0p1", "disk/")},
		{Cmd: exec.Command("extlinux", "-i", "disk/")},
		{Cmd: exec.Command("cp", "../sys/src/9/amd64/harvey.32bit", "disk/harvey")},
		{Cmd: exec.Command("ls", "-l", "disk")},
	}
	part = `label: dos
label-id: 0xadc9f7b1
device: /dev/nbd0
unit: sectors

/dev/nbd0p1 : start=        2048, size=     6289408, type=83, bootable
`
	syslinuxconfig = []byte(`# syslinux.cfg
#TIMEOUT = Time to wait to autoboot in 1/10 secs. zero (0) disables the timeout 
#PROMPT = one (1) displays the prompt only, zero (0) will not display the prompt
#ONTIMEOUT = The default menu label to automatically boot at timeout (selected after timeout)
#DISPLAY = Optional menu text if not using LABEL Entries

TIMEOUT 1000
DEFAULT vesamenu.c32
#MENU BACKGROUND /backgroundimages/your_image_here.png
MENU TITLE Harvey OS
PROMPT 0
ONTIMEOUT 1
#DISPLAY Menu.txt

#Display help text when F1 pressed on Syslinux Menu
F1 help.txt

LABEL 0
MENU LABEL Reboot
TEXT HELP
Restart the computer
ENDTEXT
COM32 reboot.c32

LABEL 1
TEXT HELP
Boot Harvey kernel
ENDTEXT
MENU LABEL Harvey OS
KERNEL mboot.c32
APPEND ../harvey

`)
)

func detach() {
	log.Println("Detaching")
	if err := exec.Command("umount", "disk").Run(); err != nil {
		log.Print(err)
	}
	if err := exec.Command("qemu-nbd", "-d", "/dev/nbd0").Run(); err != nil {
		log.Print(err)
	}
}

func main() {

	if os.Getuid() != 0 {
		log.Fatalf("You have to run me as root")
	}

	defer detach()

	for _, c := range setupcmds {
		c.Stdin, c.Stdout, c.Stderr = c.in, os.Stdout, os.Stderr
		if err := c.Run(); err != nil {
			log.Fatalf("%v: %v", c, err)
		}
	}

	fi, err := ioutil.ReadDir("img/syslinux-bios/syslinux")
	if err != nil {
		log.Fatal(err)
	}
	if err := os.Mkdir("disk/syslinux", 0777); err != nil {
		log.Print(err)
	}
	for _, f := range fi {
		log.Printf("Process %v", f)
		b, err := ioutil.ReadFile(filepath.Join("img/syslinux-bios/syslinux", f.Name()))
		if err != nil {
			log.Fatal(err)
		}
		if err := ioutil.WriteFile(filepath.Join("disk/syslinux/", f.Name()), b, 0666); err != nil {
			log.Fatal(err)
		}
	}
	if err := ioutil.WriteFile(filepath.Join("disk/syslinux/", "syslinux.cfg"), syslinuxconfig, 0666); err != nil {
		log.Fatal(err)
	}
}
