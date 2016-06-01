load("//sys/src/harvey.BUILD", "CMD_DEPS", "harvey_binary")

DEPS =[
	":devtab",
	"//sys/src/libusb:libusb",
]

USBD_DEPS = DEPS+ CMD_DEPS 

harvey_binary(
	name = "usbd",
	includes=[
		"//sys/include",
		"//amd64/include",
		"//sys/src/cmd/usb/usbd",
	],
	deps=USBD_DEPS,
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