struct fonttab{
	int index, size, width, height, left;
	double fwid,basetotop;
};
void mkchar(struct dict *, struct object, struct fonttab *, int []);
