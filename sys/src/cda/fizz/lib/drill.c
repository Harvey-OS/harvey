/*
	first two entries are special!!
	item 0: default pin-hole
	item 1: default vb pin
*/
static struct dinfo
{
	char drill;
	char *comment;
	short diam;
} drills[] = {
	'A', "55 mil round pin", 55,
	'V', "nail head for vb pins", 30,
	0
};

int
dofd(char d)
{
	register i;

	for(i = 0; drills[i].drill; i++)
		if(drills[i].drill == d)
			return(drills[i].diam);
	return(0);
}

char
dfltphd(void)
{
	return(drills[0].drill);
}

char
dfltvbd(void)
{
	return(drills[1].drill);
}
