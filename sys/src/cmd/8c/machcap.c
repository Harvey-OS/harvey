#include "gc.h"

int
machcap(Node *n)
{
//	return 0;

	if(n == Z)
		return 1;	/* test */

	switch(n->op) {
	case OMUL:
	case OLMUL:
	case OASMUL:
	case OASLMUL:
		if(typechl[n->type->etype])
			return 1;
		if(typev[n->type->etype]) {
//		if(typev[n->type->etype] && n->right->op == OCONST) {
//			if(hi64v(n->right) == 0)
				return 1;
		}
		break;

	case OCOM:
	case ONEG:
	case OADD:
	case OAND:
	case OOR:
	case OSUB:
	case OXOR:
	case OASHL:
	case OLSHR:
	case OASHR:
		if(typechlv[n->left->type->etype])
			return 1;
		break;

	case OCAST:
		if(typev[n->type->etype]) {
			if(typechlp[n->left->type->etype])
				return 1;
		}
		else if(!typefd[n->type->etype]) {
			if(typev[n->left->type->etype])
				return 1;
		}
		break;

	case OCOND:
	case OCOMMA:
	case OLIST:
	case OANDAND:
	case OOROR:
	case ONOT:
	case ODOT:
		return 1;

	case OASADD:
	case OASSUB:
	case OASAND:
	case OASOR:
	case OASXOR:
		return 1;

	case OASASHL:
	case OASASHR:
	case OASLSHR:
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
//print("%O\n", n->op);
		return 1;
	}
	return 0;
}
