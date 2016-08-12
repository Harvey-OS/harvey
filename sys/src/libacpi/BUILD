

cc_library(
    name="libacpi",
    includes=[
        "//sys/include/acpi/acpica",
        "//sys/src/9",
        "//sys/include",
        "//amd64/include",
    ],
    srcs=glob(["acpica/components/*/*.c"]),
    copts=[
        "-DACPI_DEBUGGER",
        "-DACPI_DISASSEMBLER",
        "-DACPI_DEBUG_OUTPUT",
        "-D__HARVEY__",
        "-U_LINUX",
        "-U__linux__",
        "-Wno-unused-function",
        "-Wno-unused-variable",
        "-nostdlib",
        "-fno-builtin",
        "-include", "u.h",
        "-include", "libc.h",
        "-include", "ctype.h",
        "-include", "mach_acpi.h",
    ],
)
