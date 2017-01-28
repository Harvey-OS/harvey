load('//$ARCH/include/FLAGS', "LIB_COMPILER_FLAGS")

cc_library(
	name = "libndb",
        copts = LIB_COMPILER_FLAGS,
        includes=[
            "//sys/include",
            "//amd64/include",
	    "//sys/src/libndb"
        ],
	srcs = [
		"csgetval.c",
		"csipinfo.c",
		"dnsquery.c",
		"ipattr.c",
		"ndbaux.c",
		"ndbcache.c",
		"ndbcat.c",
		"ndbconcatenate.c",
		"ndbdiscard.c",
		"ndbfree.c",
		"ndbgetipaddr.c",
		"ndbgetval.c",
		"ndbhash.c",
		"ndbipinfo.c",
		"ndblookval.c",
		"ndbopen.c",
		"ndbparse.c",
		"ndbreorder.c",
		"ndbsubstitute.c"
	]
)
