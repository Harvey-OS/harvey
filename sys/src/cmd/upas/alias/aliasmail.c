#include "common.h"

/*
 *  WARNING!  This turns all upper case names into lower case
 *  local ones.
 */

/* predeclared */
static String	*getdbfiles(void);
static int	translate(char*, char*, String*, String*);
static int	lookup(char*, String*, 	String*, String*);
static int	compare(String*, char*);
static char*	mklower(char*);

static int debug;
#define DEBUG if(debug)

/* loop through the names to be translated */
void
main(int argc, char *argv[])
{
	String *alias;		/* the alias for the name */
	char *thissys;		/* name of this system */
	String *files;		/* list of files to search */
	int i, rv;

	ARGBEGIN {
	case 'd':
		debug = 1;
		break;
	} ARGEND
	if (chdir(MAILROOT) < 0) {
		perror("translate(chdir):");
		exit(1);
	}
	if (chdir("lib") < 0) {
		perror("translate(chdir):");
		exit(1);
	}

	/* get environmental info */
	thissys = sysname_read();
	files = getdbfiles();
	alias = s_new();

	/* loop through the names to be translated (from standard input) */
	for(i=0; i<argc; i++) {
		mklower(argv[i]);
		rv = translate(argv[i], thissys, files, alias);
		if (rv < 0 || *s_to_c(alias) == '\0')
			print("local!%s\n", argv[i]);
		else
			print("%s\n", s_to_c(alias));
	}
	exits(0);
}

/* get the list of dbfiles to search */
static String *
getdbfiles(void)
{
	Biobuf *fp;
	String *files = s_new();

	/* system wide aliases */
	if ((fp = sysopen("namefiles", "r", 0)) != 0){
		while(s_getline(fp, files))
			s_append(files, " ");
		sysclose(fp);
	}


	DEBUG print("files are %s\n", s_to_c(files));

	return files;
}

/* loop through the translation files */
static int
translate(char *name,		/* name to translate */
	char *thissys,		/* name of this system */
	String *files,
	String *alias)		/* where to put the alias */
{
	String *file = s_new();
	String *fullname;
	char *user;

	DEBUG print("translate(%s, %s, %s, %s)\n", name, thissys,
		s_to_c(files), s_to_c(alias));

	/* create the full name to avoid loops (system!name) */
	fullname = s_copy(thissys);
	s_append(fullname, "!");
	s_append(fullname, name);

	/* look at user's local names */
	user = getlog();
	if (user != 0) {
		mboxpath("names", user, s_restart(file), 0);
		if (lookup(name, fullname, file, alias)==0) {
			s_free(fullname);
			s_free(file);
			return 0;
		}
	}

	/* look at system-wide names */
	s_restart(files);
	while (s_parse(files, s_restart(file)) != 0) {
		if (lookup(name, fullname, file, alias)==0) {
			s_free(fullname);
			s_free(file);
			return 0;
		}
	}

	return -1;
}

/*  Loop through the entries in a translation file looking for a match.
 *  Return 0 if found, -1 otherwise.
 */
static int
lookup(	char *name,
	String *fullname,
	String *file,
	String *alias)	/* returned String */
{
	Biobuf *fp;
	String *line = s_new();
	String *token = s_new();
	int rv = -1;

	DEBUG print("lookup(%s, %s, %s, %s)\n", name, s_to_c(fullname),
		s_to_c(file), s_to_c(alias));

	s_reset(alias);
	if ((fp = sysopen(s_to_c(file), "r", 0)) == 0)
		return -1;

	/* look for a match */
	while (s_getline(fp, s_restart(line))!=0) {
		DEBUG print("line is %s\n", s_to_c(line));
		s_restart(token);
		if (s_parse(s_restart(line), token)==0)
			continue;
		if (compare(token, name)!=0)
			continue;
		/* match found, get the alias */
		while(s_parse(line, s_restart(token))!=0) {
			/* avoid definition loops */
			if (compare(token, name)==0 ||
			    compare(token, s_to_c(fullname))==0){
				s_append(alias, "local");
				s_append(alias, "!");
				s_append(alias, name);
			} else {
				s_append(alias, s_to_c(token));
			}
			s_append(alias, " ");
		}
		rv = 0;
		break;
	}
	s_free(line);
	s_free(token);
	sysclose(fp);
	return rv;
}

#define lower(c) ((c)>='A' && (c)<='Z' ? (c)-('A'-'a'):(c))

/* compare two Strings (case insensitive) */
static int
compare(String *s1,
	char *p2)
{
	char *p1 = s_to_c(s1);
	int rv;

	DEBUG print("comparing %s to %s\n", p1, p2);
	while((rv = lower(*p1) - lower(*p2)) == 0) {
		if (*p1 == '\0')
			break;
		p1++;
		p2++;
	}
	return rv;
}

char*
mklower(char *name)
{
	char *p;
	char c;

	for(p = name; *p; p++){
		c = *p;
		*p = lower(c);
	}
	return name;
}
