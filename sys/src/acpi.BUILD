CMD_DEPS = [
	"//sys/src/libacpi:libacpi",
	"//sys/src/libstdio:libstdio",
	"//sys/src/libc:libc",
]

CMD_LINK_OPTS = [
	"-static",
	"-e_main",
]

LIB_COMPILER_FLAGS = [
		        "-DACPI_DEBUGGER",
		        "-DACPI_DISASSEMBLER",
		        "-DACPI_EXEC_APP",
		        "-D__HARVEY__",
		        "-U_LINUX",
		        "-U__linux__",
                        "-U__FreeBSD__",
			"-Wno-unused-function",
			"-Wno-unused-variable",
			"-Wno-unused-const-variable",
			"-Wno-unknown-pragmas",
			"-Wno-unused-variable",
			"-Wall", 
			"-Werror", 
			"-nostdlib",
			"-nostdinc",
		        "-fno-builtin",
			"-include", "u.h",
			"-include", "libc.h",
			"-include", "ctype.h",
			"-include", "mach_acpi.h",
]


acpi_binary = cc_binary(
	copts=LIB_COMPILER_FLAGS,
	includes=[
		"//sys/include",
		"//amd64/include",
		"//sys/include/acpi/acpica"
	],
	deps=CMD_DEPS,
	linkopts=CMD_LINK_OPTS
)
