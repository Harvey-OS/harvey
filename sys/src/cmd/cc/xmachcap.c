#include	"cc.h"

/* default, like old cc */
int
machcap(Node* n)
{
	if(n == Z)
		return 0;
	switch(n->op){
	case OCOMMA:
	case OCOND:
	case OLIST:
		return 1;
	}
	return 0;
}
