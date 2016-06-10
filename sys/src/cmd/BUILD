load('//sys/src/FLAGS', "LIB_COMPILER_FLAGS", "CMD_LINK_OPTS")

# Order of libs
# "/$ARCH/lib/libString.a",
# "/$ARCH/lib/lib9p.a",
# "/$ARCH/lib/libndb.a",
# "/$ARCH/lib/libauth.a",
# "/$ARCH/lib/libbio.a",
# "/$ARCH/lib/libip.a",
# "/$ARCH/lib/libc.a"

CMDS=[
        "//sys/src/cmd/acme:acme",
        "//sys/src/cmd/rc:rc",
        "//sys/src/cmd/ip/ipconfig:ipconfig",
        ":echo",
        ":mount",
        ":date",
        ":srv",
        ":ls",
        ":bind",
    ]

group(
    name="cmds",
    deps=CMDS
)

cc_binary(
    name="echo",
    copts=LIB_COMPILER_FLAGS,
    srcs=[
        "echo.c"
    ],
    deps=[
        "//sys/src/libc:libc",
    ],
    includes=[
        "//sys/include",
        "//amd64/include",
    ],
    linkopts=CMD_LINK_OPTS
)

cc_binary(
    name="mount",
    copts=LIB_COMPILER_FLAGS,
    srcs=[
        "mount.c"
    ],
    deps=[
        "//sys/src/libauth:libauth",
        "//sys/src/libc:libc",
    ],
    includes=[
        "//sys/include",
        "//amd64/include",
    ],
    linkopts=CMD_LINK_OPTS
)

cc_binary(
    name="date",
    copts=LIB_COMPILER_FLAGS,
    srcs=[
        "date.c"
    ],
    deps=[
        "//sys/src/libc:libc",
    ],
    includes=[
        "//sys/include",
        "//amd64/include",
    ],
    linkopts=CMD_LINK_OPTS
)

cc_binary(
    name="cat",
    copts=LIB_COMPILER_FLAGS,
    srcs=[
        "cat.c"
    ],
    deps=[
        "//sys/src/libc:libc",
    ],
    includes=[
        "//sys/include",
        "//amd64/include",
    ],
    linkopts=CMD_LINK_OPTS
)

cc_binary(
    name="bind",
    copts=LIB_COMPILER_FLAGS,
    srcs=[
        "bind.c"
    ],
    deps=[
        "//sys/src/libc:libc",
    ],
    includes=[
        "//sys/include",
        "//amd64/include",
    ],
    linkopts=CMD_LINK_OPTS
)

cc_binary(
    name="srv",
    copts=LIB_COMPILER_FLAGS,
    srcs=[
        "srv.c"
    ],
    deps=[
        "//sys/src/libauth:libauth",
        "//sys/src/libc:libc",
    ],
    includes=[
        "//sys/include",
        "//amd64/include",
    ],
    linkopts=CMD_LINK_OPTS
)

cc_binary(
    name="ls",
    copts=LIB_COMPILER_FLAGS,
    srcs=[
        "ls.c"
    ],
    deps=[
        "//sys/src/libbio:libbio",
        "//sys/src/libc:libc",
    ],
    includes=[
        "//sys/include",
        "//amd64/include",
    ],
    linkopts=CMD_LINK_OPTS
)

cc_binary(
    name="ps",
    copts=LIB_COMPILER_FLAGS,
    srcs=[
        "ps.c"
    ],
    deps=[
        "//sys/src/libbio:libbio",
        "//sys/src/libc:libc",
    ],
    includes=[
        "//sys/include",
        "//amd64/include",
    ],
    linkopts=CMD_LINK_OPTS
)

cc_binary(
    name="ed",
    copts=LIB_COMPILER_FLAGS,
    srcs=[
        "ed.c"
    ],
    deps=[
     	"//sys/src/libregexp:libregexp",
        "//sys/src/libbio:libbio",
        "//sys/src/libc:libc",
    ],
    includes=[
        "//sys/include",
        "//amd64/include",
        "//sys/src/libregexp",
    ],
    linkopts=CMD_LINK_OPTS
)

cc_binary(
    name="ratrace",
    copts=LIB_COMPILER_FLAGS,
    srcs=[
        "ratrace.c"
    ],
    deps=[
	"//sys/src/libthread:libthread",
	"//sys/src/libc:libc",
    ],
    includes=[
        "//sys/include",
        "//amd64/include",
    ],
    linkopts=CMD_LINK_OPTS
)
