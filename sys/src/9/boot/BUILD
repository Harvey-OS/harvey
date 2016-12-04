load("//sys/src/FLAGS", "LIB_COMPILER_FLAGS")

ARCHS = [
	"amd64",
	"riscv",
	"aarch64",
]

[mk_sys(
    name=printf("boot%scpuc", arch),
    mode=printf("boot%scpu.c", arch),
    arch=arch,
    sysconf="//sys/src/sysconf.json",
) for arch in ARCHS]

[data_to_c(
    name=printf("%scpu", arch),
    bin=printf("bin/boot%scpu", arch),
    prefix=printf("ramfs_boot%scpu_", arch),
    deps=[
        printf(":boot%scpu", arch),
    ]
) for arch in ARCHS]

cc_library(
    name="libboot",
    copts=[
        "-g",
        "-Wall",
        "-Wno-missing-braces",
        "-Wno-parentheses",
        "-Wno-unknown-pragmas",
        "-O0",
        "-static",
        "-mno-red-zone",
        "-ffreestanding",
        "-fno-builtin",
        "-mcmodel=small"
    ],
    includes=[
        "//sys/include",
        "//sys/src/9/boot",
        printf("//%s/include", env("ARCH")),
    ],
    deps=[
        printf(":boot%scpuc",  env("ARCH")),
    ],
    srcs=[
        "aux.c",
        "bootauth.c",
        "boot.c",
        "bootcache.c",
        "bootip.c",
        printf("boot%scpu.c", env("ARCH")),
        "bootrc.c",
        "embed.c",
        "local.c",
        "paq.c",
        "sac.c",
        "settime.c"
    ]
) 

[cc_binary(
    name=printf("boot%scpu", arch),
    copts=LIB_COMPILER_FLAGS,
    includes=[
        "//sys/include",
        "//sys/src/9/boot",
        printf("//%s/include",arch),
    ],
    srcs=[
        "printstub.c",
        printf("boot%scpu.c", arch), 
    ],
    deps=[
        ":libboot",
        printf(":boot%scpuc", arch),
        "//sys/src/libip:libip",
        "//sys/src/libauth:libauth",
        "//sys/src/libc:libc"
    ],
    linkopts=[
        "-e_main",
        "-static"
    ]
) for arch in ARCHS]