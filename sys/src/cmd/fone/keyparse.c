#include "all.h"
#include <ctype.h>

char *
dialparse(int smart, char *out, char *p, int size)
{
	int len;
	char nbuf[32];
	char *q = &nbuf[2];
	int chatty = 0;

	len = keyparse(q, p, sizeof nbuf-2);
	if(len == 0)
		return 0;
	if(smart)switch(len){
	case 12:	/* 912015826215 */
		if(strncmp(&nbuf[2], "91", 2) == 0)
			break;
		/* fall through */
	default:	/* 9011 or 011 international number */
		if(len <= 7)
			return 0;
		if(strncmp(&nbuf[2], "9011", 4) == 0)
			break;
		if(strncmp(&nbuf[2], "011", 3) != 0)
			return 0;
		/* fall through */
	case 3:		/* 411 */
	case 7:		/* 5826215 */
	case 11:	/* 12015826215 */
		*--q = '9';
		break;
	case 4:		/* 6215 */
		break;
	case 10:	/* 2015826215 */
		*--q = '1';
		*--q = '9';
		break;
	}
	if(chatty)
		print("dialparse: \"%s\"\n", q);
	return strncpy(out, q, size);
}

static uchar dtable[] = {
	/*000*/ 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
	/*010*/ 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
	/*020*/ 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
	/*030*/ 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
	/*040*/ 0x00, 0x00, 0x00,  '#', 0x00, 0x00, 0x00, 0x00,
	/*050*/ 0x00, 0x00,  '*', 0x00, 0x00, 0x00, 0x00, 0x00,
	/*060*/  '0',  '1',  '2',  '3',  '4',  '5',  '6',  '7',
	/*070*/  '8',  '9', 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/*100*/ 0x00,  '2',  '2',  '2',  '3',  '3',  '3',  '4',
	/*110*/  '4',  '4',  '5',  '5',  '5',  '6',  '6',  '6',
	/*120*/  '7',  '7',  '7',  '7',  '8',  '8',  '8',  '9',
	/*130*/  '9',  '9',  '9', 0x00, 0x00, 0x00, 0x00, 0x00,
	/*140*/ 0x00,  '2',  '2',  '2',  '3',  '3',  '3',  '4',
	/*150*/  '4',  '4',  '5',  '5',  '5',  '6',  '6',  '6',
	/*160*/  '7',  '7',  '7',  '7',  '8',  '8',  '8',  '9',
	/*170*/  '9',  '9',  '9', 0x00, 0x00, 0x00, 0x00, 0x80
};

int
keyparse(char *out, char *in, int size)
{
	int c, len = 0;
	char *q = out;
	int chatty = 0;

	if(chatty)
		print("keyparse:");
	while(c = (*in++ & 0xff)){	/* assign = */
		if(chatty)
			print(" %2.2x", c);
		if(c&0x80)
			goto error;
		c = dtable[c];
		if(chatty)
			print("->%2.2x", c);
		if(c&0x80)
			goto error;
		if(c == 0)
			continue;
		*q++ = c;
		if(++len >= size)
			goto error;
	}
	*q = 0;
	if(chatty)
		print("\nkeyparse: %d: \"%s\"\n", len, out);
	if(len <= 0)
		return 0;
	return len;
error:
	if(chatty)
		print(" len,size=%d,%d\n", len,size);
	return 0;
}

int
fieldparse(char *lp, char **fields, int n, int sep)
{
	int i;

	for(i=0; lp && *lp && i<n; i++){
		while(*lp == sep)
			*lp++ = 0;
		if(*lp == 0)
			break;
		fields[i] = lp;
		while(*lp && *lp != sep)
			lp++;
	}
	return i;
}
