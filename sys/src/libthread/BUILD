load('//sys/src/FLAGS', "LIB_COMPILER_FLAGS")

cc_library(
	name = "libthread",
        copts = LIB_COMPILER_FLAGS,
        includes=[
            "//sys/include",
            "//amd64/include",
        ],
	srcs = [
		"$ARCH.c",
		"channel.c",
		"chanprint.c",
		"create.c",
		"debug.c",
		"dial.c",
		"exec.c",
		"exit.c",
		"id.c",
		"iocall.c",
		"ioclose.c",
		"iodial.c",
		"ioopen.c",
		"ioproc.c",
		"ioread.c",
		"ioreadn.c",
		"iosleep.c",
		"iowrite.c",
		"kill.c",
		"lib.c",
		"main.c",
		"note.c",
		"ref.c",
		"rendez.c",
		"sched.c"
	]
)
