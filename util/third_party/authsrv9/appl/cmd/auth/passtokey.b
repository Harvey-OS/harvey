implement Passtokey;

include "sys.m";
	sys: Sys;
	sprint: import sys;
include "draw.m";
include "arg.m";
include "auth9.m";
	auth9: Auth9;

Passtokey: module {
	init:	fn(nil: ref Draw->Context, args: list of string);
};


init(nil: ref Draw->Context, args: list of string)
{
	sys = load Sys Sys->PATH;
	arg := load Arg Arg->PATH;
	auth9 = load Auth9 Auth9->PATH;
	auth9->init();

	arg->init(args);
	arg->setusage(arg->progname()+" password");
	while((c := arg->opt()) != 0)
		case c {
		* =>	arg->usage();
		}
	args = arg->argv();
	if(len args != 1)
		arg->usage();
	key := auth9->passtokey(hd args);
	if(sys->write(sys->fildes(1), key, len key) != len key)
		fail(sprint("write: %r"));
}

fail(s: string)
{
	sys->fprint(sys->fildes(2), "%s\n", s);
	raise "fail:"+s;
}
