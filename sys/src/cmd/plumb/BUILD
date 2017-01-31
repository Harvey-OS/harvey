load('//sys/src/harvey.BUILD', "harvey_binary")

harvey_binary(
	name="plumb",
	includes=[
		"//amd64/include",
		"//sys/include",
	],
	srcs=[
		"plumb.c",
	],
)
harvey_binary(
	name="plumber",
	includes=[
		"//amd64/include",
		"//sys/include",
		"//sys/src/cmd/plumb",
	],
	srcs=[
		"fsys.c",
		"match.c",
		"plumber.c",
		"rules.c",
	],
)
