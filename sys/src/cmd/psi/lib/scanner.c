#include <u.h>
#include <libc.h>
#include <ctype.h>
#include "system.h"
# include "stdio.h"
# include "defines.h"
# include "object.h"
#include "class.h"

struct	object
scanner(FILE *fp)
{
	struct object object;
	int brace=0;

	if(saveflag != 0){
		saveflag=0;
		return(saveobj);
	}
	object = get_token(fp, &brace);
	switch(brace){
	case 1:
		return(scan_proc(fp));
	case 2:
		error("unbalanced }");
	default:
		return(object);
	}
}
struct	object
scan_proc(FILE *fp)
{
	struct object object;
	int brace=0;
	markOP() ;

	while(1){
		object = get_token(fp, &brace);
		switch(brace){
		case 1:
			push(scan_proc(fp));
			brace = 0;
			break;
		case 2:
			counttomarkOP() ;
			arrayOP() ;
			astoreOP() ;
			cvxOP() ;
			exchOP() ;
			pop() ;
			return(pop()) ;
		default:
			if(object.type == OB_NONE)
				pserror("syntaxerror eof", "scan_proc") ;
			else push(object);
		}
	}

}

