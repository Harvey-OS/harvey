#include <stdio.h>
#include <pwd.h>

static char PASSWD[]	= "/etc/passwd";
static FILE *pwf = NULL;
static char line[BUFSIZ+2];
static struct passwd passwd;

setpwent()
{
	if( pwf == NULL )
		pwf = fopen( PASSWD, "r" );
	else
		rewind( pwf );
}

endpwent()
{
	if( pwf != NULL ){
		fclose( pwf );
		pwf = NULL;
	}
}

static char *
pwskip(p)
register char *p;
{
	while( *p && *p != ':' && *p != '\n' )
		++p;
	if( *p ) *p++ = 0;
	else p[1] = 0;
	return(p);
}

struct passwd *
pwdecode(p)
	register char *p;
{
	passwd.pw_name = p;
	p = pwskip(p);
	p = pwskip(p); /* passwd */
	passwd.pw_uid = atoi(p);
	p = pwskip(p);
	passwd.pw_gid = atoi(p);
	p = pwskip(p); /* comment */
	p = pwskip(p); /* gecos */
	passwd.pw_dir = p;
	p = pwskip(p);
	passwd.pw_shell = p;
	p = pwskip(p);
	return(&passwd);
}

struct passwd *
getpwent()
{
	register char *p;

	if (pwf == NULL) {
		if( (pwf = fopen( PASSWD, "r" )) == NULL )
			return(0);
	}
	p = fgets(line, BUFSIZ, pwf);
	if (p==NULL)
		return(0);
	return pwdecode (p);
}
