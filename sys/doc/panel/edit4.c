void snarf(void){
	int s0, s1;
	Rune *text;
	plegetsel(edit, &s0, &s1);
	if(s0==s1) return;
	text=pleget(edit);
	if(snarfbuf) free(snarfbuf);
	nsnarfbuf=s1-s0;
	snarfbuf=malloc(nsnarfbuf*sizeof(Rune));
	memmove(snarfbuf, text+s0, nsnarfbuf*sizeof(Rune));
}
