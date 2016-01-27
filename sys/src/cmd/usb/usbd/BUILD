load("//sys/src/FLAGS",  "CMD_DEPS", "CMD_LINK_OPTS", "LIB_COMPILER_FLAGS")

DEPS =[
	":devtab",
	"//sys/src/libusb:libusb",
]

USBD_DEPS = DEPS+ CMD_DEPS 

cc_binary(
	name = "usbd",
	copts = LIB_COMPILER_FLAGS,
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
	linkopts=CMD_LINK_OPTS,
)

usb(
	name="devtab",
	conf="usbdb.json",
)