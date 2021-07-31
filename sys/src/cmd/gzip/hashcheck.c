
static int
hashstr(uchar *s)
{
	ulong cont;

	cont = (s[0] << 16) | (s[1] << 8) | s[2];
	return hashit(cont) & HashMask;
}

/*
 * check that all the hash list invariants are really satisfied
 */
static void
hashcheck(LZstate *lz, char *where)
{
	int *hash, *nexts;
	uchar found[MaxOff], *s;
	int i, age, a, now, then;

	nexts = lz->nexts;
	hash = lz->hash;
	now = lz->now;
	memset(found, 0, sizeof(found));
	for(i = 0; i < HashSize; i++){
		age = 0;
		for(then = hash[i]; now-then <= MaxOff; then = nexts[then & (MaxOff-1)]){
			a = now - then;
			if(a < age)
				sysfatal("%s: out of order links age %d a %d now %d then %d",
					where, age, a, now, then);
			age = a;

			s = lz->hist + (then & (HistBlock-1));

			if(hashstr(s) != i)
				sysfatal("%s: bad hash then=%d now=%d s=%ld chain=%d hash %d %d %d",
					where, then, now, s - lz->hist, i, hashstr(s - 1), hashstr(s), hashstr(s + 1));

			if(found[then & (MaxOff - 1)])
				sysfatal("%s: found link again", where);
			found[then & (MaxOff - 1)] = 1;
		}
	}

	for(then = now - (MaxOff-1); then != now; then++)
		found[then & (MaxOff - 1)] = 1;

	for(i = 0; i < MaxOff; i++)
		if(!found[i] && nexts[i] != 0)
			sysfatal("%s: link not found: max %d at %d", where, now & (MaxOff-1), i);
}
