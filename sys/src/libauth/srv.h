/*
 * routines common to server authentication routines
 */

static void
netchown(char *user)
{
	Dir d;

	if(dirfstat(0, &d) < 0)
		return;
	strncpy(d.uid, user, NAMELEN);
	strncpy(d.gid, user, NAMELEN);
	d.mode = 0600;
	dirfwstat(0, &d);
}
