load('//sys/src/harvey.BUILD', "harvey_binary")
load('//sys/src/FLAGS', "CMD_LINK_OPTS")

CMD_DEPS = [
    "//sys/src/lib9p:lib9p",
    "//sys/src/libauth:libauth",
    "//sys/src/libauthsrv:libauthsrv",
    "//sys/src/libcomplete:libcomplete",
    "//sys/src/libcontrol:libcontrol",
    "//sys/src/libdisk:libdisk",
    "//sys/src/libdraw:libdraw",
    "//sys/src/libflate:libflate",
    "//sys/src/libframe:libframe",
    "//sys/src/libgeometry:libgeometry",
    "//sys/src/libhttpd:libhttpd",
    "//sys/src/libip:libip",
    "//sys/src/liblex:liblex",
    "//sys/src/libmach:libmach",
    "//sys/src/libmemlayer:libmemlayer",
    "//sys/src/libmemdraw:libmemdraw",
    "//sys/src/libmp:libmp",
    "//sys/src/libndb:libndb",
    "//sys/src/libplumb:libplumb",
    "//sys/src/libregexp:libregexp",
    "//sys/src/libsec:libsec",
    "//sys/src/libstdio:libstdio",
    "//sys/src/libString:libString",
    "//sys/src/libthread:libthread",
    "//sys/src/libusb:libusb",
    "//sys/src/libventi:libventi",
    "//sys/src/liboventi:liboventi",
    "//sys/src/libbio:libbio",
    "//sys/src/libc:libc",
]


harvey_binary(
    name="rio",
    srcs=[
        "rio.c",
        "data.c",
        "fsys.c",
        "scrl.c",
        "time.c",
        "util.c",
        "wctl.c",
        "wind.c",
        "xfid.c"
    ],
    deps=CMD_DEPS,
    linkopts=CMD_LINK_OPTS,
)
