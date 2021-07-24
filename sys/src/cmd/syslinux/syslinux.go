package main

import (
	"bytes"
	"flag"
	"io"
	"io/ioutil"
	"log"
	"os"
	"os/exec"
	"path/filepath"
)

var (
	part = `label: dos
label-id: 0xadc9f7b1
device: /dev/nbd0
unit: sectors

/dev/nbd0p1 : start=        2048, size=      102400, type=83, bootable
/dev/nbd0p2 : start=      104448, size=     6187008, type=39
`
	syslinuxconfig = []byte(`
TIMEOUT 50
DEFAULT vesamenu.c32
#DEFAULT 0
MENU TITLE Harvey OS
PROMPT 0
ONTIMEOUT 0

LABEL 0
TEXT HELP
Boot Harvey kernel with local filesystem
ENDTEXT
MENU LABEL Harvey OS (Local FS)
KERNEL mboot.c32
APPEND ../harvey service=cpu nobootprompt=local!#S/sdE0/ maxcores=1024 nvram=/boot/nvram nvrlen=512 nvroff=0 acpiirq=1 mouseport=ps2 vgasize=1024x768x24 monitor=vesa

LABEL 1
TEXT HELP
Boot Harvey kernel with remote filesystem (10.0.2.2)
ENDTEXT
MENU LABEL Harvey OS (Remote FS)
KERNEL mboot.c32
APPEND ../harvey service=cpu nobootprompt=tcp maxcores=1024 fs=10.0.2.2 auth=10.0.2.2 nvram=/boot/nvram nvrlen=512 nvroff=0 acpiirq=1 mouseport=ps2 vgasize=1024x768x24 monitor=vesa

LABEL 2
TEXT HELP
Install harvey on image
ENDTEXT
MENU LABEL Harvey OS (Install)
KERNEL mboot.c32
APPEND ../harvey service=cpu nobootprompt=tcp maxcores=1024 fs=10.0.2.2 auth=10.0.2.2 nvram=/boot/nvram nvrlen=512 nvroff=0 acpiirq=1 mouseport=ps2 vgasize=1024x768x24 monitor=vesa onstartup=/util/img/prepfs
`)
)

type command struct {
	*exec.Cmd
	in io.Reader
}

func gencmds(root, imgType string) []command {
	imgFilename := "harvey." + imgType
	mbr := filepath.Join(root, "util/img/syslinux-bios/mbr.bin")
	return []command{
		{Cmd: exec.Command("qemu-img", "create", "-f", imgType, imgFilename, "3G")},
		{Cmd: exec.Command("chown", os.ExpandEnv("$SUDO_USER"), imgFilename)},
		{Cmd: exec.Command("chgrp", os.ExpandEnv("$SUDO_USER"), imgFilename)},
		{Cmd: exec.Command("modprobe", "nbd", "max_part=63")},
		{Cmd: exec.Command("qemu-nbd", "-c", "/dev/nbd0", "-f", imgType, imgFilename)},
		{Cmd: exec.Command("dd", "conv=notrunc", "bs=440", "count=1", "if="+mbr, "of=/dev/nbd0")},
		{Cmd: exec.Command("sfdisk", "/dev/nbd0"), in: bytes.NewBufferString(part)},
		{Cmd: exec.Command("mkfs.ext4", "/dev/nbd0p1")},
		{Cmd: exec.Command("mkdir", "-p", "disk/")},
		{Cmd: exec.Command("mount", "/dev/nbd0p1", "disk/")},
		{Cmd: exec.Command("extlinux", "-i", "disk/")},
		{Cmd: exec.Command("cp", filepath.Join(root, "sys/src/9/amd64/harvey.32bit"), "disk/harvey")},
		{Cmd: exec.Command("ls", "-l", "disk")},
	}
}

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
	root := os.Getenv("HARVEY")
	if root == "" {
		log.Fatalf("$HARVEY environment variable is not set")
	}
	imgType := flag.String("imgtype", "raw", "Type of image to create (raw, qcow2)")
	flag.Parse()
	if *imgType != "qcow2" && *imgType != "raw" {
		log.Println("Invalid imgtype")
		return
	}

	if os.Getuid() != 0 {
		log.Println("You have to run me as root")
		return
	}

	defer detach()

	setupcmds := gencmds(root, *imgType)
	for _, c := range setupcmds {
		log.Printf("cmd: %v", c)
		c.Stdin, c.Stdout, c.Stderr = c.in, os.Stdout, os.Stderr
		if err := c.Run(); err != nil {
			log.Printf("%v: %v", c.Cmd, err)
			return
		}
	}

	dir := filepath.Join(root, "util/img/syslinux-bios/syslinux")
	fi, err := ioutil.ReadDir(dir)
	if err != nil {
		log.Print(err)
		return
	}
	if err := os.Mkdir("disk/syslinux", 0777); err != nil {
		log.Print(err)
	}
	for _, f := range fi {
		log.Printf("Process %v", f)
		b, err := ioutil.ReadFile(filepath.Join(dir, f.Name()))
		if err != nil {
			log.Print(err)
			return
		}
		if err := ioutil.WriteFile(filepath.Join("disk/syslinux/", f.Name()), b, 0666); err != nil {
			log.Print(err)
			return
		}
	}
	if err := ioutil.WriteFile(filepath.Join("disk/syslinux/", "syslinux.cfg"), syslinuxconfig, 0666); err != nil {
		log.Print(err)
		return
	}
}
