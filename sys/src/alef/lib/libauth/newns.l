#include <alef.h>
#include <bio.h>
#include <auth.h>

enum
{
	NARG	= 15,		/* max number of arguments */
	MAXARG	= 10*NAMELEN,	/* max length of an argument */
};

intern int	setenv(byte*, byte*);
intern byte*	expandarg(byte*, byte*);
intern int	splitargs(byte*, byte**, byte*, int);
intern void	nsop(int, byte**, int);
intern int	callexport(byte*, byte*);

int
newns(byte *user, byte *file)
{
	Biobuf *spec;
	byte home[2*NAMELEN], *cmd;
	byte *argv[NARG], argbuf[MAXARG*NARG];
	int argc;
	int afd;

	/* try for authentication server now because later is impossible */
	if(strcmp(user, "none") == 0)
		afd = -1;
	else
		afd = authdial();
	if(!file)
		file = "/lib/namespace";
	spec = Bopen(file, OREAD);
	if(spec == nil){
		werrstr("can't open %s: %r", file);
		close(afd);
		return -1;
	}
	rfork(RFENVG|RFCNAMEG);
	setenv("user", user);
	sprint(home, "/usr/%s", user);
	setenv("home", home);

	while(cmd = spec->rdline('\n')){
		cmd[spec->linelen()-1] = '\0';
		while(*cmd==' ' || *cmd=='\t')
			cmd++;
		if(*cmd == 0 || *cmd == '#')
			continue;
		argc = splitargs(cmd, argv, argbuf, NARG);
		if(argc)
			nsop(argc, argv, afd);
	}
	spec->term();
	close(afd);
	return 0;
}

intern void
nsop(int argc, byte **argv, int afd)
{
	byte *argv0, *a;
	uint flags;
	int fd;

	flags = 0;
	argv0 = *argv++;
	argc--;
	for(; a = *argv; argv++){
		if(*a++ != '-')
			break;
		while(*a){
			switch(*a){
			case 'a':
				flags |= MAFTER;
				break;
			case 'b':
				flags |= MBEFORE;
				break;
			case 'c':
				flags |= MCREATE;
				break;
			}
			a++;
		}
		argc--;
	}

	if(!(flags & (MAFTER|MBEFORE)))
		flags |= MREPL;

	if(strcmp(argv0, "bind") == 0 && argc == 2)
		bind(argv[0], argv[1], flags);
	if(strcmp(argv0, "mount") == 0){
		fd = open(argv[0], ORDWR);
		authenticate(fd, afd);
		if(argc == 2){
			mount(fd, argv[1], flags, "");
		}else if(argc == 3){
			mount(fd, argv[1], flags, argv[2]);
		}
		close(fd);
	}
	if(strcmp(argv0, "import") == 0){
		fd = callexport(argv[0], argv[1]);
		authenticate(fd, afd);
		if(argc == 2)
			mount(fd, argv[1], flags, "");
		else if(argc == 3)
			mount(fd, argv[2], flags, "");
		close(fd);
	}
	if(strcmp(argv0, "cd") == 0 && argc == 1)
		chdir(argv[0]);
}

intern int
callexport(byte *sys, byte *tree)
{
	byte *na, buf[3];
	int fd;

	na = netmkaddr(sys, nil, "exportfs");
	if((fd = dial(na, nil, nil, nil)) < 0)
		return -1;
	if(auth(fd) < 0 || write(fd, tree, strlen(tree)) < 0
	|| read(fd, buf, 3) != 2 || buf[0]!='O' || buf[1]!= 'K'){
		close(fd);
		return -1;
	}
	return fd;
}

intern int
splitargs(byte *p, byte **argv, byte *argbuf, int maxargs)
{
	byte *q;
	int i;

	i = 0;
	while(i < maxargs){
		while(*p==' ' || *p=='\t')
			p++;
		if(!*p)
			return i;
		q = p;
		while(*p && *p!=' ' && *p!='\t')
			p++;
		if(*p)
			*p++ = '\0';
		argv[i++] = argbuf;
		argbuf = expandarg(q, argbuf);
		if(!argbuf)
			return 0;
	}
	return 0;
}

/*
 * copy the arg into the buffer,
 * expanding any environment variables.
 * environment variables are assumed to be
 * names (ie. < NAMELEN long)
 * the entire argument is expanded to be at
 * most MAXARG long and null terminated
 * the address of the byte after the terminating null is returned
 * any problems cause a 0 return;
 */
intern byte *
expandarg(byte *arg, byte *buf)
{
	byte env[3+NAMELEN], *p, *q;
	int fd, n, len;

	n = 0;
	while(p = utfrune(arg, '$')){
		len = p - arg;
		if(n + len + NAMELEN >= MAXARG-1)
			return nil;
		memmove(&buf[n], arg, len);
		n += len;
		p++;
		arg = utfrune(p, '\0');
		q = utfrune(p, '/');
		if(q != nil && q < arg)
			arg = q;
		q = utfrune(p, '$');
		if(q != nil && q < arg)
			arg = q;
		len = arg - p;
		if(len >= NAMELEN)
			continue;
		strcpy(env, "#e/");
		strncpy(env+3, p, len);
		env[3+len] = '\0';
		fd = open(env, OREAD);
		if(fd >= 0){
			len = read(fd, &buf[n], NAMELEN - 1);
			if(len > 0)
				n += len;
			close(fd);
		}
	}
	len = strlen(arg);
	if(n + len >= MAXARG - 1)
		return nil;
	strcpy(&buf[n], arg);
	return &buf[n+len+1];
}

intern int
setenv(byte *name, byte *val)
{
	byte ename[NAMELEN+6];
	int s, f;

	sprint(ename, "#e/%s", name);
	f = create(ename, OWRITE, 0664);
	if(f < 0)
		return -1;
	s = strlen(val);
	if(write(f, val, s) != s){
		close(f);
		return -1;
	}
	close(f);
	return 0;
}
