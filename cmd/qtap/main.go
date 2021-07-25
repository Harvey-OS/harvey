package main

import (
	"fmt"
	"log"
	"os"
	"time"

	"github.com/u-root/u-root/pkg/qemu"
)

func main() {
	os.Setenv("UROOT_QEMU", "qemu-system-x86_64 -m 512");
	opts := &qemu.Options{
		Kernel:       os.ExpandEnv("$HARVEY/sys/src/9/amd64/harvey.32bit"),
		SerialOutput: os.Stdout,
		KernelArgs:   "service=terminal nobootprompt=tcp maxcores=1024 nvram=/boot/nvram nvrlen=512 nvroff=0 acpiirq=1",
	}
	log.Println(opts.Cmdline())
	vm, err := opts.Start()
	if err != nil {
		log.Fatal()
	}
	if err := vm.ExpectTimeout("Hello, I am Harvey :-)", time.Minute); err != nil {
		log.Fatal(err)
	}
	vm.Close()
	fmt.Println("Harvey booted successfully")
}
