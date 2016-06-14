load("//sys/src/cmd/BUILD", "CMDS")
objcopy(
	name="harvey",
	deps=[
		"//sys/src/9/amd64:harvey"
	],
	infile="elf64-x86-64",
	outfile="elf32-i386",
)
kernel(
	name="kernel",
	deps=
		":harvey",
		CMDS
	],
	installs={
		"amd64/harvey": "bin/harvey",
		"amd64/bin/init": "bin/init",
		"amd64/bin/echo": "bin/echo",
		"amd64/bin/ls": "bin/ls",
		"amd64/bin/cat": "bin/cat",
		"amd64/bin/date": "bin/date",
		"amd64/bin/ipconfig": "bin/ipconfig",
		"amd64/bin/acme": "bin/acme",
	},
)
