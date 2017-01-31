group(
	name="libsncmds",
	prefix=env("ARCH"),
	deps=[
		"//sys/src/cmd:cmds",
		"//sys/src:libs",
	],
)
ARCH = env("ARCH")
group(
	name="kernel",
	deps=[
		"//sys/src/9/%s:kernelbins" %  ARCH,
		":libsncmds",
	],
)
