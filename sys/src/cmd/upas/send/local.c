#include "common.h"
#include "send.h"

/*
 *  Check forwarding requests
 */
extern dest *
expand_local(dest *dp)
{
	Biobuf *fp;
	String *file = s_new();
	String *line = s_new();
	char *cp;
	dest *rv;

	/*
	 *  look for `forward' file for forwarding address(es)
	 */
	if (dp->repl1 == 0){
		mboxpath("forward", s_to_c(dp->addr), file, 0);
	} else {
		mboxpath(s_to_c(dp->repl1), s_to_c(dp->addr), file, 0);
		cp = strrchr(s_to_c(file), '/');
		if(cp)
			file->ptr = cp+1;
		else
			file->ptr = file->base;
		s_append(file, "forward");
	}
	fp = sysopen(s_to_c(file), "lr", 0);
	if (fp != 0) {
		s_read_line(fp, line);
		sysclose(fp);
		if(debug)
			fprint(2, "forward = %s\n", s_to_c(line));
		rv = s_to_dest(s_restart(line), dp);
		if(rv == 0)
			dp->status = d_badmbox;
		s_free(line);
		s_free(file);
		return rv;
	}

	/*
	 *  see if the mailbox directory exists
	 */
	if (dp->repl1 == 0){
		mboxpath(".", s_to_c(dp->addr), s_reset(file), 0);
	} else {
		mboxpath(s_to_c(dp->repl1), s_to_c(dp->addr), s_reset(file), 0);
		cp = strrchr(s_to_c(file), '/');
		if(cp)
			file->ptr = cp+1;
		else
			file->ptr = file->base;
		s_append(file, ".");
	}
	if(sysexist(s_to_c(file)))
		dp->status = d_cat;
	else
		dp->status = d_unknown;
	s_free(line);
	s_free(file);
	return 0;
}
