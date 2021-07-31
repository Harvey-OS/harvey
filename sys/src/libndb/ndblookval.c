#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ip.h>
#include <ndb.h>

/*
 *  Look for a pair with the given attribute.  look first on the same line,
 *  then in the whole entry.
 */
Ndbtuple*
ndblookvalue(Ndbtuple *entry, Ndbtuple *line, char *attr, char *to, int len)
{
	Ndbtuple *nt;

	/* first look on same line (closer binding) */
	werrstr("");
	for(nt = line;;){
		if(strcmp(attr, nt->attr) == 0){
			strncpy(to, nt->val, len);
			to[len-1] = 0;
			if(strlen(nt->val) >= len)
				werrstr("return value truncated");
			return nt;
		}
		nt = nt->line;
		if(nt == line)
			break;
	}

	/* search whole tuple */
	for(nt = entry; nt; nt = nt->entry)
		if(strcmp(attr, nt->attr) == 0){
			strncpy(to, nt->val, len);
			to[len-1] = 0;
			if(strlen(nt->val) >= len)
				werrstr("return value truncated");
			return nt;
		}
	return 0;
}

Ndbtuple*
ndblookval(Ndbtuple *entry, Ndbtuple *line, char *attr, char *to)
{
	return ndblookvalue(entry, line, attr, to, Ndbvlen);
}
