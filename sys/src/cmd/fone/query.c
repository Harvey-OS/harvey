#include "all.h"
#include <ctype.h>

#define	NARGV	3

typedef struct Querymsg{
	int	key;
	char *	msg;
}Querymsg;

enum{
	Q_other, Q_ready, Q_proc
};

Querymsg qmsgs[] = {
	Q_ready, "DIRECTORY - READY FOR QUERY NAME",
	Q_proc, "PROCESSING REQUEST",
	Q_other, 0
};

void
querystart(char *buf)
{
	Call *cp = callsN;
	int argc; char *argv[NARGV], *p;
	int n;

	if(cp->state != Null){
		print("query already in progress\n");
		return;
	}
	cp->id = cp-calls;
	for(p=buf; *p; p++)
		*p = toupper(*p);
	argc = fieldparse(buf, argv, NARGV, ' ');
	memset(cp->who, 0, sizeof cp->who);
	switch(argc){
	default:
		return;
	case 1:	/* killian */
		if(debug)
			print("%s\n", argv[0]);
		break;
	case 2:	/* t killian or tj killian */
		if(debug)
			print("%s %s\n", argv[0], argv[1]);
		cp->who[0] = argv[0][0];
		cp->who[1] = argv[0][1];
		break;
	case 3:	/* t j killian */
		if(debug)
			print("%s %s\n", argv[0], argv[1], argv[2]);
		cp->who[0] = argv[0][0];
		cp->who[1] = argv[1][0];
		break;
	}
	strncpy(&cp->who[3], argv[argc-1], sizeof cp->who-4);
	n = keyparse(cp->num, &cp->who[3], sizeof cp->num-4);
	if(n > 0 && cp->who[0] != 0){
		cp->num[n++] = '0';
		n = keyparse(&cp->num[n], cp->who, 3);
	}
	if(n <= 0){
		print("can't parse query name\n");
		return;
	}
	strcat(cp->num, "#");
	memset(cp->tty, 0, sizeof cp->tty);
	cp->dialp = cp->num;
	cp->state = Query_req;
	cp->flags = 0;

	putcmd("AT*B%d\r", Startquery);
}

int
queryinfo(char *m)
{
	Call *cp = callsN;
	Querymsg *q;
	char *s, *e;
	int c, len;

	if(cp->state != Query_req){
		print("%s\n", m);
		return 0;
	}
		
	len = strlen(m);
	if(len > 9 && isdigit(m[0]) && isdigit(m[1]) && m[2] == '/' &&
	   isdigit(m[3]) && isdigit(m[4]) && !isdigit(m[5])){
		if(m[5] == ' ')
			s = &m[6];
		else
			s = &m[5];
		c = memcmp(&s[3], &cp->who[3], strlen(&cp->who[3]));
		if(c == 0 && cp->who[0])
			c = memcmp(&s[0], &cp->who[0], 1);
		if(c == 0 && cp->who[1])
			c = memcmp(&s[1], &cp->who[1], 1);
		if(c==0){
			e = &m[len-1];
			while(e > s && *e == ' ')
				e--;
			print("%.*s\n", e-s+1, s);
			cp->flags |= Altered;
		}
		if(strncmp(&m[0], &m[3], 2) != 0 && c <= 0)
			putcmd("AT*B%d\r", Nextquery);
		else{
			putcmd("AT*B%d\r", Startquery);
			cp->state = Null;
			if(!(cp->flags&Altered))
				print("No matching records\n");
		}
		return 0;
	}
	for(q=qmsgs; q->msg; q++){
		if(strcmp(m, q->msg) == 0)
			break;
	}
	switch(q->key){
	case Q_ready:
		putcmd("AT%s%s\r", dquerycmd, cp->dialp);
		cp->dialp = 0;
		break;
	case Q_proc:
		break;
	case Q_other:
		print("%s\n", m);
		putcmd("AT*B%d\r", Startquery);
		cp->state = Null;
		return 1;
	}
	return 0;
}
