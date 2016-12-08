load("//sys/src/FLAGS", "LIB_COMPILER_FLAGS")

ARCHS = [
	"amd64",
	"riscv",
	"aarch64",
]

[mk_sys(
	name="boot%scpuc" % arch,
	mode="boot%scpu.c" % arch,
	arch=arch,
	sysconf="//sys/src/sysconf.json",
) for arch in ARCHS]

[data_to_c(
	name="%scpu" % arch,
	bin="bin/boot%scpu" %arch,
	prefix= "ramfs_boot%scpu_" % arch,
	deps=[
		":boot%scpu" % arch,
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
		fmt("//%s/include", env("ARCH")),
	],
	deps=[
		fmt(":boot%scpuc", env("ARCH")),
	],
	srcs=[
		"aux.c",
		"bootauth.c",
		"boot.c",
		"bootcache.c",
		"bootip.c",
	fmt("boot%scpu.c", env("ARCH")),
		"bootrc.c",
		"embed.c",
		"local.c",
		"paq.c",
		"sac.c",
		"settime.c"
	]
)

[cc_binary(
	name="boot%scpu" % arch,
	copts=LIB_COMPILER_FLAGS,
	includes=[
		"//sys/include",
		"//sys/src/9/boot",
		"//%s/include" % arch,
	],
	srcs=[
		"printstub.c",
		"boot%scpu.c" % arch,
	],
	deps=[
		":libboot",
		":boot%scpuc" % arch,
		"//sys/src/libip:libip",
		"//sys/src/libauth:libauth",
		"//sys/src/libc:libc"
	],
	linkopts=[
		"-e_main",
		"-static"
	]
) for arch in ARCHS]
