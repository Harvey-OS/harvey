load('//sys/src/FLAGS', "LIB_COMPILER_FLAGS")

cc_library(
	name = "libauth",
        copts = LIB_COMPILER_FLAGS,
        includes=[
            "//sys/include"
            "//amd64/include"
        ],
	srcs = [
		"amount.c",
		"amount_getkey.c",
		"attr.c",
		"auth_attr.c",
		"auth_challenge.c",
		"auth_chuid.c",
		"auth_getkey.c",
		"auth_getuserpasswd.c",
		"auth_proxy.c",
		"auth_respond.c",
		"auth_rpc.c",
		"auth_userpasswd.c",
		"auth_wep.c",
		"login.c",
		"newns.c",
		"noworld.c"
	]
)
