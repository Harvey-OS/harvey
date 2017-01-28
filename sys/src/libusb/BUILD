load('//$ARCH/include/FLAGS', "LIB_COMPILER_FLAGS")

cc_library(
	name = "libusb",
        copts = LIB_COMPILER_FLAGS,
        includes=[
            "//sys/include",
            "//amd64/include",
        ],
	srcs = [
		"lib/dev.c",
		"lib/devs.c",
		"lib/dump.c",
		"lib/parse.c",
		"lib/fs.c",
		"lib/fsdir.c",
		"audio/audio.c",
		"audio/audioctl.c",
		"audio/audiofs.c",
		"audio/audiosub.c",
		"disk/disk.c",
		"disk/scsireq.c",
		"ether/asix.c",
		"ether/cdc.c",
		"ether/ether.c",
		"ether/smsc.c",
		"kb/hid.c",
		"kb/kb.c",
		"print/print.c",
		"serial/ftdi.c",
		"serial/prolific.c",
		"serial/serial.c",
		"serial/silabs.c",
		"serial/ucons.c"
	]
)
