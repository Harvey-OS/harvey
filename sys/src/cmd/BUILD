load('//sys/src/FLAGS', "KLIB_COMPILER_FLAGS")

# Order of libs
# "/$ARCH/lib/libString.a",
# "/$ARCH/lib/lib9p.a",
# "/$ARCH/lib/libndb.a",
# "/$ARCH/lib/libauth.a",
# "/$ARCH/lib/libbio.a",
# "/$ARCH/lib/libip.a",
# "/$ARCH/lib/libc.a"

group(
    name="cmds"
    deps=[
        "//sys/src/cmd/acme:acme",
    ]
)

group(
    name="kcmds"
    deps=[
        ":echo",
        ":mount",
        ":date",
        ":srv",
        ":ls",
        "//sys/src/cmd/rc:rc",
        "//sys/src/cmd/ip/ipconfig:ipconfig"
    ]
)

KCMD_LINK_OPTS = [
    "-static",
    "-e_main"
]

cc_binary(
    name="echo",
    copts=KLIB_COMPILER_FLAGS,
    srcs=[
        "echo.c"
    ],
    deps=[
        "//sys/src/libc:libc",
    ],
    includes=[
        "//sys/include"
        "//amd64/include"
    ],
    linkopts=KCMD_LINK_OPTS
)

cc_binary(
    name="mount",
    copts=KLIB_COMPILER_FLAGS,
    srcs=[
        "mount.c"
    ],
    deps=[
        "//sys/src/libauth:libauth",
        "//sys/src/libc:libc",
    ],
    includes=[
        "//sys/include"
        "//amd64/include"
    ],
    linkopts=KCMD_LINK_OPTS
)

cc_binary(
    name="date",
    copts=KLIB_COMPILER_FLAGS,
    srcs=[
        "date.c"
    ],
    deps=[
        "//sys/src/libc:libc",
    ],
    includes=[
        "//sys/include"
        "//amd64/include"
    ],
    linkopts=KCMD_LINK_OPTS
)

cc_binary(
    name="srv",
    copts=KLIB_COMPILER_FLAGS,
    srcs=[
        "srv.c"
    ],
    deps=[
        "//sys/src/libauth:libauth",
        "//sys/src/libc:libc",
    ],
    includes=[
        "//sys/include"
        "//amd64/include"
    ],
    linkopts=KCMD_LINK_OPTS
)

cc_binary(
    name="ls",
    copts=KLIB_COMPILER_FLAGS,
    srcs=[
        "ls.c"
    ],
    deps=[
        "//sys/src/libbio:libbio",
        "//sys/src/libc:libc",
    ],
    includes=[
        "//sys/include"
        "//amd64/include"
    ],
    linkopts=KCMD_LINK_OPTS
)
