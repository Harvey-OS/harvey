load('//sys/src/FLAGS', "LIB_COMPILER_FLAGS", "CMD_LINK_OPTS")

harvey_binary = cc_binary(
    copts=LIB_COMPILER_FLAGS,
    includes=[
        "//sys/include",
        "//amd64/include",
    ],
    deps=[
        "//sys/src/libc:libc",
    ],
    linkopts=CMD_LINK_OPTS
)