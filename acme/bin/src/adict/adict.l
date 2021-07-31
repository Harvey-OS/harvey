#include <alef.h>
#include <bio.h>
#include "win.h"
#include "adict.h"

byte *prog = "adict";
byte *lprog = "/bin/adict";
byte *xprog  = "/bin/dict";
byte *dict, *pattern, *curaddr[MAXMATCH], *curone, *args[6], buffer[80];
byte abuffer[80], fbuffer[80], pbuffer[80];
int curindex, count, Eopen, Mopen;
Win Mwin, Ewin, Dwin;

void openwin(byte*, byte*, Win*, int);
void  handle(Win*, int);
void  rexec(byte *, byte**, int*, chan(int));
int getaddr(byte*);

void
main(int argc, byte** argv)
{
  dict = pattern = nil;
  count = 0;
    
  if (argc>=3 && strcmp(argv[1],"-d")==0){
	--argc;
	--argc;
	dict = malloc(strlen(argv[2]));
	strcpy(dict, argv[2]);
	argv[1] = argv[3];
  }
  if (argc==2){
	pattern = pbuffer;
	if (dict == nil)  dict = "oed";		/* Default */
	strcpy(pattern,argv[1]);
	--argc;
  }
  if (argc != 1){
	fprint(2, "usage: %s [-d dictname] [pattern]\n",prog);
	exits(nil);
  }

  if ((dict == nil) && (pattern == nil))
	openwin(prog,"", &Dwin, Dictwin);
  if (pattern == nil)
	openwin(prog,"",&Ewin, Entrywin);
  if ((count = getaddr(pattern)) <= 1)
	openwin(prog,"Prev Next", &Ewin, Entrywin);
  else
   	openwin(prog, "Quit", &Mwin, Matchwin);
}

int
getaddr(byte *pattern)
{
  /* Get byte offset into dictionary of matches. */

  int fpipe[2], i;
  chan(int) c;
  Biobuf inbuf;
  byte *bufptr;

  if (pattern == nil) {
	curone = nil;
	curindex = 0;
	curaddr[curindex] = nil;
	return 0;
  }
  alloc c;
  pipe(fpipe);
  sprint(buffer,"/%s/A", pattern);
  args[0] = xprog;
  args[1] = "-d";
  args[2] = dict;
  args[3] = "-c";
  args[4] = buffer;
  args[5] = nil;
  proc rexec(xprog, args, fpipe, c); 
   <-c;			/* Synchronize with child! */
  close(fpipe[1]);
  inbuf.init(fpipe[0], OREAD);
  i = 0;
  curindex = 0;
  while ((bufptr = inbuf.rdline('\n')) != nil && (i < (MAXMATCH-1))) {
	bufptr[inbuf.linelen()-1] = 0;
	while (bufptr[0] != '#') bufptr++;
	curaddr[i] = malloc(strlen(bufptr));
	strcpy(curaddr[i], bufptr);
	i++;
  }
  curaddr[i] = nil;
  if (i == MAXMATCH)
	fprint(2, "Too many matches!\n");
  inbuf.term();
  curone = curaddr[curindex];
  return(i);
}

byte*
getpattern(byte *addr)
{  
  /* Get the pattern corresponding to an absolute address.*/
  int fpipe[2];
  chan(int) c;
  byte *res, *t;

  res = nil;
  alloc c;
  pipe(fpipe);
  sprint(buffer,"%sh", addr);		
  args[0] = xprog;
  args[1] = "-d";
  args[2] = dict;
  args[3] = "-c";
  args[4] = buffer;
  args[5] = nil;
  proc rexec(xprog, args, fpipe, c); 
   <-c;			/* Synchronize with child! */
  close(fpipe[1]);
  if (read(fpipe[0], pbuffer, 80) > 80) 
	fprint(2, "Error in getting addres from dict.\n");
  else {
	t = pbuffer;
	while ((t[0] != '\n') && (t != nil)) t++;
	if (t != nil) t[0] = 0;
	pbuffer[strlen(pbuffer) - 1] = 0;	/* Remove mysterious extra char. */
	res = pbuffer;
  }
  close(fpipe[0]);
  return(res);
}

byte*
chgaddr(int dir)
{
  /* Increment or decrement the current address (curone). */

  int fpipe[2];
  chan(int) c;
  byte *res, *t;

  res = nil;
  alloc c;
  pipe(fpipe);
  if (dir < 0)
  	sprint(buffer,"%s-a", curone);
  else
	sprint(buffer,"%s+a", curone);		
  args[0] = xprog;
  args[1] = "-d";
  args[2] = dict;
  args[3] = "-c";
  args[4] = buffer;
  args[5] = nil;
  proc rexec(xprog, args, fpipe, c); 
   <-c;		/* Synchronize with child! */
  close(fpipe[1]);
  if (read(fpipe[0], abuffer, 80) > 80) 
	fprint(2, "Error in getting addres from dict.\n");
  else {
	res = abuffer;
	while (*res != '#') res++;
	t = res;
	while ((*t != '\n') && (t != nil)) t++;
	if (t != nil) *t = 0;
  } 
  close(fpipe[0]);
  return(res);
}

void
dispdicts(Win *cwin)
{
  /* Display available dictionaries in window. */

  int fpipe[2], nb, i;
  chan(int) c;
  byte buf[1024], *t;

  alloc c;
  pipe(fpipe);
  args[0] = xprog;
  args[1] = "-d";
  args[2] = "?";
  args[3] = nil;
  proc rexec(xprog, args, fpipe, c); 
  <-c;			/* Synchronize with child! */
  close(fpipe[1]);
  cwin->wreplace("0,$","",0);	/* Clear window */
  while ((nb = read(fpipe[0], buf, 1024)) > 0) {
	t = buf;
	i = 0;
	if (strncmp("Usage", buf, 5) == 0) {	/* Remove first line. */
		while (t[0] != '\n') {t++; i++;}
		t++; i++;
	}
  	cwin->wwritebody(t, nb-i);
  }
  close(fpipe[0]);
  cwin->wclean();
}

void
dispentry(Win *cwin)
{
  /* Display the current selection in window. */

  int fpipe[2], nb;
  chan(int) c;
  byte buf[BUFSIZE];

  if (curone == nil) {
	if (pattern != nil) {
		sprint(buf,"Pattern not found.\n");
		cwin->wwritebody(buf, 19);
		cwin->wclean();
	}
	return;
  }
  alloc c;
  pipe(fpipe);
  sprint(buffer,"%sp", curone);
  args[0] = xprog;
  args[1] = "-d";
  args[2] = dict;
  args[3] = "-c";
  args[4] = buffer;
  args[5] = nil;
  proc rexec(xprog, args, fpipe, c); 
  <-c;			/* Synchronize with child! */
  close(fpipe[1]);
  cwin->wreplace("0,$","",0);	/* Clear window */
  while ((nb = read(fpipe[0], buf, BUFSIZE)) > 0) {
  	cwin->wwritebody(buf, nb);
  }
  close(fpipe[0]);
  cwin->wclean();
}

void
dispmatches(Win *cwin)
{
  /* Display the current matches. */

  int fpipe[2], nb;
  chan(int) c;
  byte buf[BUFSIZE];

  alloc c;
  pipe(fpipe);
  sprint(buffer,"/%s/H", pattern);
  args[0] = xprog;
  args[1] = "-d";
  args[3] = "-c";
  args[4] = buffer;
  args[5] = nil;
  proc rexec(xprog, args, fpipe, c);
   <-c;			/* Synchronize with child! */
  close(fpipe[1]);
  while ((nb = read(fpipe[0], buf, BUFSIZE)) > 0)
  	cwin->wwritebody(buf, nb);
  close(fpipe[0]);
  cwin->wclean();
}

byte*
format(byte *s)
{
  /* Format a string to be written in window tag.  Acme doesn't like */
  /* non alpha-num's in the tag line. */

  byte *t, *h;

  t = fbuffer;
  if (s == nil) {
	*t = 0;
	return t;
  }
  strcpy(t, s);
  h = t;
  while (*t != 0) {
	if (!(((*t >= 'a') && (*t <= 'z')) || 
	      ((*t >= 'A') && (*t <= 'Z')) ||
	      ((*t >= '0') && (*t <= '9'))))
			*t = '_';
	t++;
  }
  if (strlen(h) > MAXTAG)
	h[MAXTAG] = 0;
  if (strcmp(s,h) == 0) return s;
  return h;
}

void
openwin(byte *name, byte *buttons, Win *twin, int wintype)
{
  byte buf[80];
  chan(Event) wevent;

  alloc wevent;

  twin->wnew();
  if (wintype == Dictwin)
	  sprint(buf,"%s",name);
  else
  	if ((wintype == Entrywin) && (count > 1))
	  sprint(buf,"%s/%s/%s/%d",name, dict, format(pattern), curindex+1);
  	else
  	  sprint(buf,"%s/%s/%s",name, dict, format(pattern));
  twin->wname(buf);
  twin->wtagwrite(buttons, strlen(buttons));
  twin->wclean();
  twin->wdormant();
  if (wintype == Dictwin)
	dispdicts(twin);
  if (wintype == Matchwin) {
	Mopen = True;
	dispmatches(twin);
  }
  if (wintype == Entrywin) {
	Eopen = True;
	dispentry(twin);
  }
  handle(twin, wintype);
}


void
rexec(byte *prog, byte **args, int fd[2], chan(int) c)
{
  rfork(RFENVG|RFFDG|RFNOTEG|RFMEM|RFNOWAIT);
  c <- = getpid();		/* Synchronize with parent! */
  dup(fd[1], 1);
  close(fd[1]);
  close(fd[0]);
  exec(prog, args);
  fprint(2, "Remote execution failed: %s", prog);
  exits(nil);
}

void pexec(byte *prog, byte **args)
{
  rfork(RFENVG|RFFDG|RFNOTEG|RFMEM|RFNOWAIT);
  exec(prog, args);
  fprint(2, "Remote execution failed: %s", prog);
  exits(nil);
}

void
kill()
{
 /* Kill all processes related to this one. */
 int fd;

  sprint(buffer, "/proc/%d/notepg", getpid());
  fd = open(buffer, OWRITE);
  rfork(RFNOTEG);
  write(fd, "kill", 4);
}

int
command(byte *com, Win *w, int wintype)
{
 byte *buf;

 if (strncmp(com, "Del", 3) == 0) {
	if (wintype == Entrywin)
		if (w->wdel()) {
			Eopen = False;
			exits(nil);
		}
	if (wintype == Dictwin)
		if (w->wdel())
			exits(nil);
	return True;
  }
  if (strncmp(com, "Quit", 4) == 0){
	kill();	
	if (Eopen)
		if (~Ewin.wdel())	/* Remove the entry window */
			Ewin.wdel();
	if (!w->wdel())
		w->wdel();
	exits(nil);
  }
  if (strncmp(com, "Next", 4) == 0){
	if (curone != nil) {
		curone = chgaddr(1);
		buf = getpattern(curone);
		sprint(buffer,"%s/%s/%s", prog, dict, format(buf));
	  	w->wname(buffer);
		dispentry(w);
	}
 	return True;
  }
  if (strncmp(com, "Prev",4) == 0){
	if (curone != nil) {
		curone = chgaddr(-1);
		buf = getpattern(curone);
		sprint(buffer,"%s/%s/%s", prog, dict, format(buf));
	  	w->wname(buffer);
		dispentry(w);
	}
	return True;
  }
  if (strncmp(com, "Nmatch",6) == 0){
	if (curaddr[++curindex] == nil)
		curindex = 0;
	curone = curaddr[curindex];
	if (curone != nil) {
		sprint(buffer,"%s/%s/%s/%d",prog,dict,format(pattern),curindex+1);
	  	w->wname(buffer);
		dispentry(w);
	}
	return True;
  }
  return False;
}

void
handle(Win *w, int wintype)
{
  Event e, e2, ea, etoss;
  byte *s, *t, buf[80];
  int tmp, na;

  while (True) {
	w->wevent(&e);
	switch(e.c2){
	default:
		/* fprint(2,"unknown message %c%c\n", e.c1, e.c2); */
		break;
	case 'i':
		/* fprint(2,"'%s' inserted in tag at %d\n", e.b, e.q0);*/
		break;
	case 'I':
		/* fprint(2,"'%s' inserted in body at %d\n", e.b, e.q0);*/
		break;
	case 'd':
		/* fprint(2, "'%s' deleted in tag at %d\n", e.b, e.q0);*/
		break;
	case 'D':
		/* fprint(2, "'%s' deleted in body at %d\n", e.b, e.q0);*/
		break;
	case 'x':
	case 'X':				/* Execute command. */
		if (e.flag & 2)
			w->wevent(&e2);
		if(e.flag & 8){
			w->wevent(&ea);
			w->wevent(&etoss);
			na = ea.nb;
		}else
			na = 0;
		s = e.b;
		if ((e.flag & 2) && e.nb == 0)
			s = e2.b;
		if(na){
			t = malloc(strlen(s)+1+na+1);
			sprint(t, "%s %s", s, ea.b);
			s = t;
		}
		/* if it's a long message, it can't be for us anyway */
		if(!command(s, w, wintype))	/* send it back */
			w->wwriteevent(&e);
		if(na)
			free(s);
		break;
	case 'l':
	case 'L':				/* Look for something. */
		if (e.flag & 2)
			w->wevent(&e);
		w->wclean();		/* Set clean bit. */
		if (wintype == Dictwin) {
		  strcpy(buf, e.b);
		  args[0] = "adict";
		  args[1] = "-d";
		  args[2] = buf;
		  args[3] = nil;
		  proc pexec(lprog, args);	/* New adict with chosen dict. */
		}
		if (wintype == Entrywin) {
		  strcpy(buf, e.b);
		  args[0] = "adict";
		  args[1] = "-d";
		  args[2] = dict;
		  args[3] = buf;
		  args[4] = nil;
		  proc pexec(lprog, args); /* New adict with chosen pattern. */
		}
		if (wintype == Matchwin) {
		   tmp = atoi(e.b) - 1;
		   if ((tmp >= 0) && (tmp < MAXMATCH) && (curaddr[tmp] != nil)) {
			curindex = tmp;
		  	curone = curaddr[curindex];
			/* Display selected match. */
			if (Eopen) {
	  		  sprint(buf,"%s/%s/%s/%d",prog,dict,format(pattern),curindex+1);
  			  Ewin.wname(buf);
			  dispentry(&Ewin);
			}
			else
			  proc openwin(prog,"Nmatch Prev Next", &Ewin, Entrywin);
		   }
		}
		break;
	}
   }
}
