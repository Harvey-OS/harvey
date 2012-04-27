#include	"gc.h"

int
machcap(Node *n)
{
	if(n == Z)
		return 1;	/* test */
	switch(n->op){

	case OADD:
	case OAND:
	case OOR:
	case OSUB:
	case OXOR:
		if(typev[n->left->type->etype])
			return 1;
		break;

	case OMUL:
	case OLMUL:
	case OASMUL:
	case OASLMUL:
		return 1;

	case OLSHR:
	case OASHR:
	case OASHL:
	case OASASHL:
	case OASASHR:
	case OASLSHR:
		return 1;

	case OCAST:
		if(typev[n->type->etype]) {
			if(!typefd[n->left->type->etype])
				return 1;
		} else if(!typefd[n->type->etype]) {
			if(typev[n->left->type->etype])
				return 1;
		}
		break;

	case OCOMMA:
	case OCOND:
	case OLIST:
	case OANDAND:
	case OOROR:
	case ONOT:
		return 1;

	case OCOM:
	case ONEG:
		if(typechl[n->left->type->etype])
			return 1;
		if(typev[n->left->type->etype])
			return 1;
		return 0;

	case OASADD:
	case OASSUB:
	case OASAND:
	case OASOR:
	case OASXOR:
		return 1;

	case OPOSTINC:
	case OPOSTDEC:
	case OPREINC:
	case OPREDEC:
		return 1;

	case OEQ:
	case ONE:
	case OLE:
	case OGT:
	case OLT:
	case OGE:
	case OHI:
	case OHS:
	case OLO:
	case OLS:
		return 1;

	case ODIV:
	case OLDIV:
	case OLMOD:
	case OMOD:
		return 0;

	case OASDIV:
	case OASLDIV:
	case OASLMOD:
	case OASMOD:
		return 0;

	}
	return 0;
}
