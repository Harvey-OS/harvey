load("//sys/src/harvey.BUILD",  "CMD_DEPS", "harvey_binary")

DEPS =[
	":devtab",
	"//sys/src/libusb:libusb",
]


harvey_binary(
	name = "usbd",
	deps=DEPS+ CMD_DEPS,
	includes=[
		"//sys/include",
		"//amd64/include",
		"//sys/src/cmd/usb/usbd",
	],
	srcs = [
		"usbd.c",
		"dev.c",
		"devtab.c",
	],
)

usb(
	name="devtab",
	conf="usbdb.json",
)