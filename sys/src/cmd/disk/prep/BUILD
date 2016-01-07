load("//sys/src/FLAGS", "LIB_COMPILER_FLAGS", "CMD_DEPS", "CMD_LINK_OPTS")
CALC = [
    ":calc"
]

PREP_DEPS = CALC + CMD_DEPS

cc_binary(
    name="fdisk",
    copts=LIB_COMPILER_FLAGS,
    includes=[
        "//sys/include",
        "//amd64/include",
        "//sys/src/cmd/disk/prep",
    ],
    deps=PREP_DEPS,
    linkopts=CMD_LINK_OPTS,
    srcs=[
        "fdisk.c",
        "edit.c",
        "y.tab.c",
    ],
)
cc_binary(
    name="prep",
    copts=LIB_COMPILER_FLAGS,
    includes=[
        "//sys/include",
        "//amd64/include",
        "//sys/src/cmd/disk/prep",
    ],
    deps=PREP_DEPS,
    linkopts=CMD_LINK_OPTS,
    srcs=[
        "prep.c",
        "edit.c",
        "y.tab.c",
    ],
)

yacc(
    name="calc",
    yaccopts=[
        "-d",
    ],
    srcs=[
        "calc.y",
    ],
    exports=[
        "y.tab.c",
    ],
)
