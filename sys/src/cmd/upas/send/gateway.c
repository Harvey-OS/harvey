#include "common.h"
#include "send.h"

#define isspace(c) ((c)==' ' || (c)=='\t' || (c)=='\n')

/*
 *  Translate the last component of the sender address.  If the translation
 *  yields the same address, replace the sender with its last component.
 */
extern void
gateway(message *mp)
{
	char *base;
	dest *dp=0;
	static Biobuf *fp;
	char *sp;

	/* first remove all systems equivalent to us */
	for (base = s_to_c(mp->sender); *base;){
		sp = strchr(base, '!');
		if(sp==0)
			break;
		*sp = '\0';
		if(lookup(base, "lib/equivlist", &fp, 0, 0)==1){
			/* found or us, forget this system */
			*sp='!';
			base=sp+1;
		} else {
			/* no files or system is not found, and not us */
			*sp='!';
			break;
		}
	}

	/* punt if this is not a compound address */
	sp = strrchr(base, '!');
	if (sp==0)
		goto rebuild;
	sp++;

	/* bind the address to a command */
	d_insert(&dp, d_new(s_copy(sp)));
	dp->authorized = 1;
	dp = up_bind(dp, mp, 0);

	/* punt if translation did not succeed or resulted in multiple targets */
	if (dp==0 || dp->next!=dp || dp->status!=d_pipe)
		goto rebuild;

	/* punt if the translation didn't result in the original address */
	if (strcmp(s_to_c(dp->addr), base)!=0)
		goto rebuild;
	base=sp;
rebuild:
	if(base!=s_to_c(mp->sender))
		mp->sender = s_copy(base);
}

static int
okfile(char *cp, Biobuf *fp)
{
	char *buf;
	int len;
	char *bp;
	int c;

	len = strlen(cp);
	
	/* one iteration per system name in the file */
	while(buf = Brdline(fp, '\n')) {
		buf[Blinelen(fp)-1] = '\0';
		for(bp=buf; *bp;){
			while(isspace(*bp) || *bp==',')
				bp++;
			if(strncmp(bp, cp, len) == 0) {
				c = *(bp+len);
				if(isspace(c) || c=='\0' || c==',')
					return 1;
			}
			while(*bp && (!isspace(*bp)) && *bp!=',')
				bp++;
		}
	}

	/* didn't find it, prohibit forwarding */
	return 0;
}

/* return 1 if name found in one of the files
 *	  0 if name not found in one of the files
 *	  -1 if neither file exists
 */
extern int
lookup(char *cp, char *local, Biobuf **lfpp, char *global, Biobuf **gfpp)
{
	static String *file = 0;

	if (local) {
		if (file == 0)
			file = s_new();
		abspath(local, MAILROOT, s_restart(file));
		if (*lfpp != 0 || (*lfpp = sysopen(s_to_c(file), "r", 0)) != 0) {
			Bseek(*lfpp, (long)0, 0);
			if (okfile(cp, *lfpp))
				return 1;
		} else
			local = 0;
	}
	if (global) {
		abspath(global, MAILROOT, s_restart(file));
		if (*gfpp != 0 || (*gfpp = sysopen(s_to_c(file), "r", 0)) != 0) {
			Bseek(*gfpp, (long)0, 0);
			if (okfile(cp, *gfpp))
				return 1;
		} else
			global = 0;
	}
	return (local || global)? 0 : -1;
}
