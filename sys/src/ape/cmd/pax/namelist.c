/* $Source: /u/mark/src/pax/RCS/namelist.c,v $
 *
 * $Revision: 1.6 $
 *
 * namelist.c - track filenames given as arguments to tar/cpio/pax
 *
 * DESCRIPTION
 *
 *	Arguments may be regular expressions, therefore all agurments will
 *	be treated as if they were regular expressions, even if they are
 *	not.
 *
 * AUTHOR
 *
 *	Mark H. Colburn, NAPS International (mark@jhereg.mn.org)
 *
 * Sponsored by The USENIX Association for public distribution. 
 *
 * Copyright (c) 1989 Mark H. Colburn.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice is duplicated in all such 
 * forms and that any documentation, advertising materials, and other 
 * materials related to such distribution and use acknowledge that the 
 * software was developed by Mark H. Colburn and sponsored by The 
 * USENIX Association. 
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * $Log:	namelist.c,v $
 * Revision 1.6  89/02/13  09:14:48  mark
 * Fixed problem with directory errors
 * 
 * Revision 1.5  89/02/12  12:14:00  mark
 * Fixed misspellings
 * 
 * Revision 1.4  89/02/12  11:25:19  mark
 * Modifications to compile and link cleanly under USG
 * 
 * Revision 1.3  89/02/12  10:40:23  mark
 * Fixed casting problems
 * 
 * Revision 1.2  89/02/12  10:04:57  mark
 * 1.2 release fixes
 * 
 * Revision 1.1  88/12/23  18:02:17  mark
 * Initial revision
 * 
 */

#ifndef lint
static char *ident = "$Id: namelist.c,v 1.6 89/02/13 09:14:48 mark Exp $";
static char *copyright = "Copyright (c) 1989 Mark H. Colburn.\nAll rights reserved.\n";
#endif /* ! lint */


/* Headers */

#include "pax.h"


/* Type Definitions */

/*
 * Structure for keeping track of filenames and lists thereof. 
 */
struct nm_list {
    struct nm_list *next;
    short           length;	/* cached strlen(name) */
    char            found;	/* A matching file has been found */
    char            firstch;	/* First char is literally matched */
    char            re;		/* regexp pattern for item */
    char            name[1];	/* name of file or rexexp */
};

struct dirinfo {
    char            dirname[PATH_MAX + 1];	/* name of directory */
    OFFSET	    where;	/* current location in directory */
    struct dirinfo *next;
};


/* Static Variables */

static struct dirinfo *stack_head = (struct dirinfo *)NULL;


/* Function Prototypes */

#ifndef __STDC__

static void pushdir();
static struct dirinfo *popdir();

#else

static void pushdir(struct dirinfo *info);
static struct dirinfo *popdir(void);

#endif


/* Internal Identifiers */

static struct nm_list *namelast;	/* Points to last name in list */
static struct nm_list *namelist;	/* Points to first name in list */


/* addname -  add a name to the namelist. 
 *
 * DESCRIPTION
 *
 *	Addname adds the name given to the name list.  Memory for the
 *	namelist structure is dynamically allocated.  If the space for 
 *	the structure cannot be allocated, then the program will exit
 *	the an out of memory error message and a non-zero return code
 *	will be returned to the caller.
 *
 * PARAMETERS
 *
 *	char *name	- A pointer to the name to add to the list
 */

#ifdef __STDC__

void add_name(char *name)

#else
    
void add_name(name)
char           *name;		/* pointer to name */

#endif
{
    int             i;		/* Length of string */
    struct nm_list *p;		/* Current struct pointer */

    i = strlen(name);
    p = (struct nm_list *) malloc((unsigned) (i + sizeof(struct nm_list)));
    if (!p) {
	fatal("cannot allocate memory for namelist entry\n");
    }
    p->next = (struct nm_list *)NULL;
    p->length = i;
    strncpy(p->name, name, i);
    p->name[i] = '\0';		/* Null term */
    p->found = 0;
    p->firstch = isalpha(name[0]);
    if (strchr(name, '*') || strchr(name, '[') || strchr(name, '?')) {
        p->re = 1;
    }
    if (namelast) {
	namelast->next = p;
    }
    namelast = p;
    if (!namelist) {
	namelist = p;
    }
}


/* name_match - match a name from an archive with a name from the namelist 
 *
 * DESCRIPTION
 *
 *	Name_match attempts to find a name pointed at by p in the namelist.
 *	If no namelist is available, then all filenames passed in are
 *	assumed to match the filename criteria.  Name_match knows how to
 *	match names with regular expressions, etc.
 *
 * PARAMETERS
 *
 *	char	*p	- the name to match
 *
 * RETURNS
 *
 *	Returns 1 if the name is in the namelist, or no name list is
 *	available, otherwise returns 0
 *
 */

#ifdef __STDC__

int name_match(char *p)

#else
    
int name_match(p)
char           *p;

#endif
{
    struct nm_list *nlp;
    int             len;

    if ((nlp = namelist) == 0) {/* Empty namelist is easy */
	return (1);
    }
    len = strlen(p);
    for (; nlp != 0; nlp = nlp->next) {
	/* If first chars don't match, quick skip */
	if (nlp->firstch && nlp->name[0] != p[0]) {
	    continue;
	}
	/* Regular expressions */
	if (nlp->re) {
	    if (wildmat(nlp->name, p)) {
		nlp->found = 1;	/* Remember it matched */
		return (1);	/* We got a match */
	    }
	    continue;
	}
	/* Plain Old Strings */
	if (nlp->length <= len	/* Archive len >= specified */
	    && (p[nlp->length] == '\0' || p[nlp->length] == '/')
	    && strncmp(p, nlp->name, nlp->length) == 0) {
	    /* Name compare */
	    nlp->found = 1;	/* Remember it matched */
	    return (1);		/* We got a match */
	}
    }
    return (0);
}


/* names_notfound - print names of files in namelist that were not found 
 *
 * DESCRIPTION
 *
 *	Names_notfound scans through the namelist for any files which were
 *	named, but for which a matching file was not processed by the
 *	archive.  Each of the files is listed on the standard error.
 *
 */

#ifdef __STDC__

void names_notfound(void)

#else
    
void names_notfound()

#endif
{
    struct nm_list *nlp;

    for (nlp = namelist; nlp != 0; nlp = nlp->next) {
	if (!nlp->found) {
	    fprintf(stderr, "%s: %s not found in archive\n",
	            myname, nlp->name);
	}
	free(nlp);
    }
    namelist = (struct nm_list *)NULL;
    namelast = (struct nm_list *)NULL;
}


/* name_init - set up to gather file names 
 *
 * DESCRIPTION
 *
 *	Name_init sets up the namelist pointers so that we may access the
 *	command line arguments.  At least the first item of the command
 *	line (argv[0]) is assumed to be stripped off, prior to the
 *	name_init call.
 *
 * PARAMETERS
 *
 *	int	argc	- number of items in argc
 *	char	**argv	- pointer to the command line arguments
 */

#ifdef __STDC__

void name_init(int argc, char **argv)

#else
    
void name_init(argc, argv)
int             argc;
char          **argv;

#endif
{
    /* Get file names from argv, after options. */
    n_argc = argc;
    n_argv = argv;
}


/* name_next - get the next name from argv or the name file. 
 *
 * DESCRIPTION
 *
 *	Name next finds the next name which is to be processed in the
 *	archive.  If the named file is a directory, then the directory
 *	is recursively traversed for additional file names.  Directory
 *	names and locations within the directory are kept track of by
 *	using a directory stack.  See the pushdir/popdir function for
 *	more details.
 *
 * 	The names come from argv, after options or from the standard input.  
 *
 * PARAMETERS
 *
 *	name - a pointer to a buffer of at least MAX_PATH + 1 bytes long;
 *	statbuf - a pointer to a stat structure
 *
 * RETURNS
 *
 *	Returns -1 if there are no names left, (e.g. EOF), otherwise returns 
 *	0 
 */

#ifdef __STDC__

int name_next(char *name, Stat *statbuf)

#else
    
int name_next(name, statbuf)
char           *name;
Stat           *statbuf;

#endif
{
    int             err = -1;
    static int      in_subdir = 0;
    static DIR     *dirp;
    struct dirent  *d;
    static struct dirinfo *curr_dir;
    int			len;

    do {
	if (names_from_stdin) {
	    if (lineget(stdin, name) < 0) {
		return (-1);
	    }
	    if (nameopt(name) < 0) {
		continue;
	    }
	} else {
	    if (in_subdir) {
		if ((d = readdir(dirp)) != (struct dirent *)NULL) {
		    /* Skip . and .. */
		    if (strcmp(d->d_name, ".") == 0 ||
		        strcmp(d->d_name, "..") == 0) {
			    continue;
		    }
		    if (strlen(d->d_name) + 
			strlen(curr_dir->dirname) >= PATH_MAX) {
			warn("name too long", d->d_name);
			continue;
		    }
		    strcpy(name, curr_dir->dirname);
		    strcat(name, d->d_name);
		} else {
		    closedir(dirp);
		    in_subdir--;
		    curr_dir = popdir();
		    if (in_subdir) {
			errno = 0;
			if ((dirp=opendir(curr_dir->dirname)) == (DIR *)NULL) {
			    warn(curr_dir->dirname, "error opening directory (1)");
			    in_subdir--;
			}
			seekdir(dirp, curr_dir->where);
		    }
		    continue;
		}
	    } else if (optind >= n_argc) {
		return (-1);
	    } else {
		strcpy(name, n_argv[optind++]);
	    }
	}
	if ((err = LSTAT(name, statbuf)) < 0) {
	    warn(name, strerror());
	    continue;
	}
	if (!names_from_stdin && (statbuf->sb_mode & S_IFMT) == S_IFDIR) {
	    if (in_subdir) {
		curr_dir->where = telldir(dirp);
		pushdir(curr_dir);
		closedir(dirp);
	    } 
	    in_subdir++;

	    /* Build new prototype name */
	    if ((curr_dir = (struct dirinfo *) mem_get(sizeof(struct dirinfo))) 
			  == (struct dirinfo *)NULL) {
		exit(2);
	    }
	    strcpy(curr_dir->dirname, name);
	    len = strlen(curr_dir->dirname);
	    while (len >= 1 && curr_dir->dirname[len - 1] == '/') {
		len--;		/* Delete trailing slashes */
	    }
	    curr_dir->dirname[len++] = '/';	/* Now add exactly one back */
	    curr_dir->dirname[len] = '\0';/* Make sure null-terminated */
            curr_dir->where = 0;
           
            errno = 0;
            do {
                if ((dirp = opendir(curr_dir->dirname)) == (DIR *)NULL) {
                     warn(curr_dir->dirname, "error opening directory (2)");
                     if (in_subdir > 1) {
                          curr_dir = popdir();
                     }
                     in_subdir--;
                     err = -1;
                     continue;
                } else {
                     seekdir(dirp, curr_dir->where);
		}
	    } while (in_subdir && (! dirp));
	}
    } while (err < 0);
    return (0);
}


/* name_gather - gather names in a list for scanning. 
 *
 * DESCRIPTION
 *
 *	Name_gather takes names from the command line and adds them to
 *	the name list.
 *
 * FIXME
 *
 * 	We could hash the names if we really care about speed here.
 */

#ifdef __STDC__

void name_gather(void)

#else
    
void name_gather()

#endif
{
     while (optind < n_argc) { 
	 add_name(n_argv[optind++]); 
     } 
}


/* pushdir - pushes a directory name on the directory stack
 *
 * DESCRIPTION
 *
 *	The pushdir function puses the directory structure which is pointed
 *	to by "info" onto a stack for later processing.  The information
 *	may be retrieved later with a call to popdir().
 *
 * PARAMETERS
 *
 *	dirinfo	*info	- pointer to directory structure to save
 */

#ifdef __STDC__

static void pushdir(struct dirinfo *info)

#else
    
static void pushdir(info)
struct dirinfo	*info;

#endif
{
    if  (stack_head == (struct dirinfo *)NULL) {
	stack_head = info;
	stack_head->next = (struct dirinfo *)NULL;
    } else {
	info->next = stack_head;
	stack_head = info;
    } 
}


/* popdir - pop a directory structure off the directory stack.
 *
 * DESCRIPTION
 *
 *	The popdir function pops the most recently pushed directory
 *	structure off of the directory stack and returns it to the calling
 *	function.
 *
 * RETURNS
 *
 *	Returns a pointer to the most recently pushed directory structure
 *	or NULL if the stack is empty.
 */

#ifdef __STDC__

static struct dirinfo *popdir(void)

#else
    
static struct dirinfo *popdir()

#endif
{
    struct dirinfo	*tmp;

    if (stack_head == (struct dirinfo *)NULL) {
	return((struct dirinfo *)NULL);
    } else {
	tmp = stack_head;
	stack_head = stack_head->next;
    }
    return(tmp);
}
