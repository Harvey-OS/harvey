#include	"all.h"

static int	getline(int);
static char	linebuf[256];

/*
 *  prompt for a string from the keyboard.  <cr> returns the default.
 */
int
getstr(char *prompt, char *buf, int size, char *def, int delay)
{
	int len;
	char *cp;

	for(;;){
		if(def)
			print("%s[%s]: ", prompt, def);
		else
			print("%s: ", prompt);
		switch(len = getline(delay)){
		case -1:
			/* ^U typed */
			continue;
		case -2:
			/* timeout */
			return -1;
		default:
			break;
		}
		if(len >= size){
			print("line too long\n");
			continue;
		}
		break;
	}
	if(*linebuf==0 && def)
		strcpy(buf, def);
	else
		strcpy(buf, linebuf);
	return 0;
}

/*
 *  read a line from the keyboard.
 */
static int
getline(int delay)
{
	int c, i, t;

	i = 0;
	t = 0;
	linebuf[0] = 0;
	delay *= GETCHZ;
	for(;;){
	    	do{
			c = getc(&kbdq);
			if(delay && t++ > delay)
				return -2;
		}while(c == -1);
		delay = 0;

		switch(c){
		case 'p' - 'a' + 1:
			panic("^p");
		case '\r':
			c = '\n';	/* turn carriage return into newline */
			break;
		case '\177':
			c = '\010';	/* turn delete into backspace */
			break;
		case '\025':
			screenputc('\n');	/* echo ^U as a newline */
			return -1;		/* return to reprompt */
		}
		screenputc(c);
		if(c == '\010'){	/* bs deletes last character */
			if(i > 0)
				linebuf[--i] = 0;
			continue;
		}
		/* a newline ends a line */
		if(c == '\n')
			return i;
		if(i < (sizeof linebuf)-1){
			linebuf[i++] = c;
			linebuf[i] = 0;
		}
	}
}
