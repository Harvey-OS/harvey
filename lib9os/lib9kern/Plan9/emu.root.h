int rootmaxq = 12;
Dirtab roottab[12] = {
	"",	{0, 0, QTDIR},	 0,	0555,
	"chan",	{1, 0, QTDIR},	 0,	0555,
	"dev",	{2, 0, QTDIR},	 0,	0555,
	"fd",	{3, 0, QTDIR},	 0,	0555,
	"prog",	{4, 0, QTDIR},	 0,	0555,
	"prof",	{5, 0, QTDIR},	 0,	0555,
	"net",	{6, 0, QTDIR},	 0,	0555,
	"net.alt",	{7, 0, QTDIR},	 0,	0555,
	"nvfs",	{8, 0, QTDIR},	 0,	0555,
	"env",	{9, 0, QTDIR},	 0,	0555,
	"root",	{10, 0, QTDIR},	 0,	0555,
	"srv",	{11, 0, QTDIR},	 0,	0555,
};
Rootdata rootdata[12] = {
	0,	 &roottab[1],	 11,	nil,
	0,	 nil,	 0,	 nil,
	0,	 nil,	 0,	 nil,
	0,	 nil,	 0,	 nil,
	0,	 nil,	 0,	 nil,
	0,	 nil,	 0,	 nil,
	0,	 nil,	 0,	 nil,
	0,	 nil,	 0,	 nil,
	0,	 nil,	 0,	 nil,
	0,	 nil,	 0,	 nil,
	0,	 nil,	 0,	 nil,
	0,	 nil,	 0,	 nil,
};
