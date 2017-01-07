load('//$ARCH/include/harvey.BUILD', "harvey_binary", "CMD_DEPS")

CALC = [
    ":calc"
]

PREP_DEPS = CALC + CMD_DEPS

harvey_binary(
    name="fdisk",
    includes=[
        "//sys/include",
        "//amd64/include",
        "//sys/src/cmd/disk/prep",
    ],
    deps=PREP_DEPS,
    srcs=[
        "fdisk.c",
        "edit.c",
        "y.tab.c",
    ],
)

harvey_binary(
    name="prep",
    includes=[
        "//sys/include",
        "//amd64/include",
        "//sys/src/cmd/disk/prep",
    ],
    deps=PREP_DEPS,
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
