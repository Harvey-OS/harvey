#include "common.h"
#include "send.h"

static void
mboxfile(dest *dp, String *path, char *file)
{
	char *cp;

	mboxpath(s_to_c(dp->repl1), s_to_c(dp->addr), path, 0);
	cp = strrchr(s_to_c(path), '/');
	if(cp)
		path->ptr = cp+1;
	else
		path->ptr = path->base;
	s_append(path, file);
}

/*
 *  Check forwarding requests
 */
extern dest*
expand_local(dest *dp)
{
	Biobuf *fp;
	String *file = s_new();
	String *line = s_new();
	dest *rv;
	int forwardok;

	/* short circuit obvious security problems */
	if(strstr(s_to_c(dp->addr), "/../")){
		dp->status = d_unknown;
		s_free(line);
		s_free(file);
		return 0;
	}

	if(dp->repl1 == 0){
		dp->repl1 = s_new();
		mboxpath("mbox", s_to_c(dp->addr), dp->repl1, 0);
	}

	/*
	 *  if this is the descendant of a `forward' file, don't
	 *  look for a forward.
	 */
	forwardok = 1;
	for(rv = dp->parent; rv; rv = rv->parent)
		if(rv->status == d_cat){
			forwardok = 0;
			break;
		}
	if(forwardok){
		/*
		 *  look for `forward' file for forwarding address(es)
		 */
		mboxfile(dp, file, "forward");
		fp = sysopen(s_to_c(file), "lr", 0);
		if (fp != 0) {
			s_read_line(fp, line);
			sysclose(fp);
			if(debug)
				fprint(2, "forward = %s\n", s_to_c(line));
			rv = s_to_dest(s_restart(line), dp);
			s_free(line);
			if(rv){
				s_free(file);
				return rv;
			}
		}
	}

	/*
	 *  look for a 'pipe' file
	 */
	mboxfile(dp, s_reset(file), "pipeto");
	if(sysexist(s_to_c(file))){
		if(debug)
			fprint(2, "found a pipeto file\n");
		dp->status = d_pipeto;
		s_append(file, " ");
		s_append(file, s_to_c(dp->addr));
		s_append(file, " ");
		s_append(file, s_to_c(dp->repl1));
		s_free(dp->repl1);
		dp->repl1 = file;
		return dp;
	}

	/*
	 *  see if the mailbox directory exists
	 */
	mboxfile(dp, s_reset(file), ".");
	if(sysexist(s_to_c(file)))
		dp->status = d_cat;
	else
		dp->status = d_unknown;
	s_free(line);
	s_free(file);
	return 0;
}
