#include "gc.h"

void
cgen(Node *n, int result, Node *nn)
{
	Node *l, *r, nod;
	int lg, rg, xg, yg, g, o;
	long v;
	Prog *p1;

	if(n == Z || n->type == T)
		return;
	if(typesuv[n->type->etype]) {
		sugen(n, result, nn, n->type->width);
		return;
	}
	if(debug['g']) {
		if(result == D_TREE)
			prtree(nn, "result");
		else
			print("result = %R\n", result);
		prtree(n, "cgen");
	}
	l = n->left;
	r = n->right;
	o = n->op;
	if(n->addable >= INDEXED) {
		if(result == D_NONE) {
			if(nn == Z)
				switch(o) {
				default:
					nullwarn(Z, Z);
					break;
				case OINDEX:
					nullwarn(l, r);
					break;
				}
			return;
		}
		gmove(n->type, nn->type, D_TREE, n, result, nn);
		return;
	}

	v = 0; /* set */
	switch(o) {
	default:
		diag(n, "unknown op in cgen: %O", o);
		break;

	case OAS:
		if(l->op == OBIT)
			goto bitas;
		/*
		 * recursive use of result
		 */
		if(result == D_NONE)
		if(l->addable > INDEXED)
		if(l->complex < FNX) {
			cgen(r, D_TREE, l);
			break;
		}

		/*
		 * function calls on both sides
		 */
		if(l->complex >= FNX && r->complex >= FNX) {
			cgen(r, D_TOS, r);
			v = argoff;
			lg = regaddr(result);
			lcgen(l, lg, Z);
			lg |= I_INDIR;
			adjsp(v - argoff);
			gmove(r->type, l->type, D_TOS, r, lg, l);
			if(result != D_NONE)
				gmove(l->type, nn->type, lg, l, result, nn);
			regfree(lg);
			break;
		}

		rg = D_TREE;
		lg = D_TREE;
		if(r->complex >= l->complex) {
			/*
			 * right side before left
			 */
			if(result != D_NONE) {
				rg = regalloc(n->type, result);
				cgen(r, rg, n);
			} else
			if(r->complex >= FNX || r->addable < INDEXED) {
				rg = regalloc(r->type, result);
				cgen(r, rg, r);
			}
			if(l->addable < INDEXED) {
				lg = regaddr(lg);
				lcgen(l, lg, Z);
				lg |= I_INDIR;
			}
		} else {
			/*
			 * left before right
			 */
			if(l->complex >= FNX || l->addable < INDEXED) {
				lg = regaddr(lg);
				lcgen(l, lg, Z);
				lg |= I_INDIR;
			}
			if(result != D_NONE) {
				rg = regalloc(n->type, result);
				cgen(r, rg, n);
			} else
			if(r->addable < INDEXED) {
				rg = regalloc(r->type, result);
				cgen(r, rg, r);
			}
		}
		if(result != D_NONE) {
			gmove(n->type, l->type, rg, r, lg, l);
			gmove(n->type, nn->type, rg, r, result, nn);
		} else
			gmove(r->type, l->type, rg, r, lg, l);
		regfree(lg);
		regfree(rg);
		break;

	bitas:
		n = l->left;
		rg = regalloc(tfield, result);
		if(l->complex >= r->complex) {
			lg = regaddr(D_NONE);
			lcgen(n, lg, Z);
			lg |= I_INDIR;
			cgen(r, rg, r);
		} else {
			cgen(r, rg, r);
			lg = regaddr(D_NONE);
			lcgen(n, lg, Z);
			lg |= I_INDIR;
		}
		g = regalloc(n->type, D_NONE);
		gmove(l->type, l->type, lg, l, g, l);
		bitstore(l, rg, lg, g, result, nn);
		break;

	case OBIT:
		if(result == D_NONE) {
			nullwarn(l, Z);
			break;
		}
		g = bitload(n, D_NONE, D_NONE, result, nn);
		gopcode(OAS, nn->type, g, n, result, nn);
		regfree(g);
		break;

	case ODOT:
		sugen(l, D_TREE, nodrat, l->type->width);
		if(result != D_NONE) {
			warn(n, "non-interruptable temporary");
			nod = *nodrat;
			if(!r || r->op != OCONST) {
				diag(n, "DOT and no offset");
				break;
			}
			nod.xoffset += r->vconst;
			nod.type = n->type;
			cgen(&nod, result, nn);
		}
		break;

	case OASLDIV:
	case OASLMOD:
	case OASDIV:
	case OASMOD:
		if(l->op == OBIT)
			goto asbitop;
		if(typefd[n->type->etype])
			goto asbinop;
		rg = D_TREE;
		if(l->complex >= FNX || r->complex >= FNX) {
			rg = D_TOS;
			cgen(r, rg, r);
			v = argoff;
		} else
		if(r->addable < INDEXED) {
			rg = regalloc(n->type, D_NONE);
			cgen(r, rg, r);
		}
		lg = D_TREE;
		if(!simplv(l)) {
			lg = regaddr(D_NONE);
			lcgen(l, lg, Z);	/* destroys register optimization */
			lg |= I_INDIR;
		}
		g = regpair(result);
		gmove(l->type, n->type, lg, l, g, n);
		if(rg == D_TOS)
			adjsp(v - argoff);
		gopcode(o, n->type, rg, r, g, n);
		if(o == OASLMOD || o == OASMOD)
			gmove(n->type, l->type, g+1, n, lg, l);
		else
			gmove(n->type, l->type, g, n, lg, l);
		if(result != D_NONE)
		if(o == OASLMOD || o == OASMOD)
			gmove(n->type, nn->type, g+1, n, result, nn);
		else
			gmove(n->type, nn->type, g, n, result, nn);
		regfree(g);
		regfree(g+1);
		regfree(lg);
		regfree(rg);
		break;

	case OASXOR:
	case OASAND:
	case OASOR:
		if(l->op == OBIT)
			goto asbitop;
		if(l->complex >= FNX ||
		   l->addable < INDEXED ||
		   result != D_NONE ||
		   typefd[n->type->etype])
			goto asbinop;
		rg = D_TREE;
		if(r->op != OCONST) {
			rg = regalloc(n->type, D_NONE);
			cgen(r, rg, r);
		}
		gopcode(o, l->type, rg, r, D_TREE, l);
		regfree(rg);
		break;

	case OASADD:
	case OASSUB:
		if(l->op == OBIT ||
		   l->complex >= FNX ||
		   l->addable < INDEXED ||
		   result != D_NONE ||
		   typefd[n->type->etype])
			goto asbinop;
		v = vconst(r);
		if(v > 0 && v <= 8) {
			gopcode(o, n->type, D_TREE, r, D_TREE, l);
			break;
		}
		rg = regalloc(n->type, D_NONE);
		cgen(r, rg, r);
		gopcode(o, n->type, rg, r, D_TREE, l);
		regfree(rg);
		break;

	case OASLSHR:
	case OASASHR:
	case OASASHL:
		if(l->op == OBIT ||
		   l->complex >= FNX ||
		   l->addable < INDEXED ||
		   result != D_NONE ||
		   typefd[n->type->etype])
			goto asbinop;
		rg = D_TREE;
		v = vconst(r);
		if(v <= 0 || v > 8) {
			rg = regalloc(n->type, D_NONE);
			cgen(r, rg, r);
		}
		lg = regalloc(n->type, D_NONE);
		cgen(l, lg, l);
		gopcode(o, n->type, rg, r, lg, l);
		gmove(n->type, n->type, lg, l, D_TREE, l);
		regfree(lg);
		regfree(rg);
		break;

	case OASLMUL:
	case OASMUL:
	asbinop:
		if(l->op == OBIT)
			goto asbitop;
		rg = D_TREE;
		if(l->complex >= FNX || r->complex >= FNX) {
			rg = D_TOS;
			cgen(r, rg, r);
			v = argoff;
		} else
		if(r->addable < INDEXED) {
			rg = regalloc(n->type, D_NONE);
			cgen(r, rg, r);
		} else {
			if(o == OASLSHR || o == OASASHR || o == OASASHL) {
				v = vconst(r);
				if(v <= 0 || v > 8) {
					rg = regalloc(n->type, D_NONE);
					cgen(r, rg, r);
				}
			}
		}
		lg = D_TREE;
		if(!simplv(l)) {
			lg = regaddr(D_NONE);
			lcgen(l, lg, Z);	/* destroys register optimization */
			lg |= I_INDIR;
		}
		g = regalloc(n->type, result);
		gmove(l->type, n->type, lg, l, g, n);
		if(rg == D_TOS)
			adjsp(v - argoff);
		if(o == OASXOR)
			if(rg == D_TREE) {
				rg = regalloc(n->type, D_NONE);
				cgen(r, rg, r);
			}
		if(o == OASXOR || o == OASLSHR || o == OASASHR || o == OASASHL)
			if(rg == D_TOS) {
				rg = regalloc(n->type, D_NONE);
				gmove(n->type, n->type, D_TOS, n, rg, n);
			}
		gopcode(o, n->type, rg, r, g, n);
		gmove(n->type, l->type, g, n, lg, l);
		if(result != D_NONE)
			gmove(n->type, nn->type, g, n, result, nn);
		regfree(g);
		regfree(lg);
		regfree(rg);
		break;

	asbitop:
		rg = regaddr(D_NONE);
		lg = regalloc(tfield, D_NONE);
		if(l->complex >= r->complex) {
			g = bitload(l, lg, rg, result, nn);
			xg = regalloc(r->type, D_NONE);
			cgen(r, xg, nn);
		} else {
			xg = regalloc(r->type, D_NONE);
			cgen(r, xg, nn);
			g = bitload(l, lg, rg, result, nn);
		}

		if(!typefd[n->type->etype]) {
			if(o == OASLDIV || o == OASDIV) {
				yg = regpair(result);
				gmove(tfield, n->type, g, l, yg, n);
				gopcode(o, n->type, xg, r, yg, n);
				gmove(n->type, tfield, yg, n, g, l);
				regfree(yg);
				regfree(yg+1);

				regfree(xg);
				bitstore(l, g, rg, lg, D_NONE, nn);
				break;
			}
			if(o == OASLMOD || o == OASMOD) {
				yg = regpair(result);
				gmove(tfield, n->type, g, l, yg, n);
				gopcode(o, n->type, xg, r, yg, n);
				gmove(n->type, tfield, yg+1, n, g, l);
				regfree(yg);
				regfree(yg+1);

				regfree(xg);
				bitstore(l, g, rg, lg, D_NONE, nn);
				break;
			}
		}

		yg = regalloc(n->type, result);
		gmove(tfield, n->type, g, l, yg, n);
		gopcode(o, n->type, xg, r, yg, n);
		gmove(n->type, tfield, yg, n, g, l);
		regfree(yg);

		regfree(xg);
		bitstore(l, g, rg, lg, D_NONE, nn);
		break;

	case OCAST:
		if(result == D_NONE) {
			nullwarn(l, Z);
			break;
		}
		lg = result;
		if(l->complex >= FNX)
			lg = regret(l->type);
		lg = eval(l, lg);
		if(nocast(l->type, n->type)) {
			gmove(n->type, nn->type, lg, l, result, nn);
			regfree(lg);
			break;
		}
		if(nocast(n->type, nn->type)) {
			gmove(l->type, n->type, lg, l, result, nn);
			regfree(lg);
			break;
		}
		rg = regalloc(n->type, result);
		gmove(l->type, n->type, lg, l, rg, n);
		gmove(n->type, nn->type, rg, n, result, nn);
		regfree(rg);
		regfree(lg);
		break;

	case OCOND:
		doinc(l, PRE);
		boolgen(l, 1, D_NONE, Z, l);
		p1 = p;

		inargs++;
		doinc(r->left, PRE);
		cgen(r->left, result, nn);
		doinc(r->left, POST);
		gbranch(OGOTO);
		patch(p1, pc);
		p1 = p;

		doinc(r->right, PRE);
		cgen(r->right, result, nn);
		doinc(r->right, POST);
		patch(p1, pc);
		inargs--;
		break;

	case OIND:
		if(result == D_NONE) {
			nullwarn(l, Z);
			break;
		}
		lg = nodalloc(types[TIND], result, &nod);
		nod.lineno = n->lineno;
		if(l->op == OADD) {
			if(l->left->op == OCONST) {
				nod.xoffset += l->left->vconst;
				l = l->right;
			} else
			if(l->right->op == OCONST) {
				nod.xoffset += l->right->vconst;
				l = l->left;
			}
		}
		cgen(l, lg, l);
		gmove(n->type, nn->type, D_TREE, &nod, result, nn);
		regfree(lg);
		break;

	case OFUNC:
		v = argoff;
		inargs++;
		gargs(r);
		lg = D_TREE;
		if(l->addable < INDEXED) {
			lg = regaddr(result);
			lcgen(l, lg, Z);
			lg |= I_INDIR;
		}
		inargs--;
		doinc(r, POST);
		doinc(l, POST);
		gopcode(OFUNC, types[TCHAR], D_NONE, Z, lg, l);
		regfree(lg);
		if(inargs)
			adjsp(v - argoff);
		if(result != D_NONE) {
			lg = regret(n->type);
			gmove(n->type, nn->type, lg, n, result, nn);
		}
		break;

	case OLDIV:
	case OLMOD:
	case ODIV:
	case OMOD:
		if(result == D_NONE) {
			nullwarn(l, r);
			break;
		}
		if(typefd[n->type->etype])
			goto binop;
		if(r->addable >= INDEXED && r->complex < FNX) {
			lg = regpair(result);
			cgen(l, lg, l);
			rg = D_TREE;
		} else {
			cgen(r, D_TOS, r);
			v = argoff;
			lg = regpair(result);
			cgen(l, lg, l);
			adjsp(v - argoff);
			rg = D_TOS;
		}
		gopcode(o, n->type, rg, r, lg, l);
		if(o == OMOD || o == OLMOD)
			gmove(l->type, nn->type, lg+1, l, result, nn);
		else
			gmove(l->type, nn->type, lg, l, result, nn);
		regfree(lg);
		regfree(lg+1);
		break;

	case OMUL:
	case OLMUL:
		if(l->op == OCONST)
			if(mulcon(r, l, result, nn))
				break;
		if(r->op == OCONST)
			if(mulcon(l, r, result, nn))
				break;
		if(debug['M'])
			print("%L multiply\n", n->lineno);
		goto binop;

	case OAND:
		if(r->op == OCONST)
		if(typeil[n->type->etype])
		if(l->op == OCAST) {
			if(typec[l->left->type->etype])
			if(!(r->vconst & ~0xff)) {
				l = l->left;
				goto binop;
			}
			if(typeh[l->left->type->etype])
			if(!(r->vconst & ~0xffff)) {
				l = l->left;
				goto binop;
			}
		}
		goto binop;

	case OADD:
		if(result == D_TOS)
		if(r->addable >= INDEXED)
		if(l->op == OCONST)
		if(typeil[l->type->etype]) {
			v = l->vconst;
			if(v > -32768 && v < 32768) {
				rg = regaddr(D_NONE);
				gmove(r->type, r->type, D_TREE, r, rg, r);
				gopcode(OADDR, types[TSHORT], D_NONE, Z, rg, r);
				p->to.offset = v;
				p->to.type |= I_INDIR;
				regfree(rg);
				break;
			}
		}

	case OSUB:
		if(result == D_TOS)
		if(l->addable >= INDEXED)
		if(r->op == OCONST)
		if(typeil[r->type->etype]) {
			v = r->vconst;
			if(v > -32768 && v < 32768) {
				if(n->op == OSUB)
					v = -v;
				lg = regaddr(D_NONE);
				gmove(l->type, l->type, D_TREE, l, lg, l);
				gopcode(OADDR, types[TSHORT], D_NONE, Z, lg, l);
				p->to.offset = v;
				p->to.type |= I_INDIR;
				regfree(lg);
				break;
			}
		}
		goto binop;

	case OOR:
	case OXOR:
	binop:
		if(result == D_NONE) {
			nullwarn(l, r);
			break;
		}
		if(l->complex >= FNX && r->complex >= FNX) {
			cgen(r, D_TOS, r);
			v = argoff;
			lg = regalloc(l->type, result);
			cgen(l, lg, l);
			adjsp(v - argoff);
			if(o == OXOR) {
				rg = regalloc(r->type, D_NONE);
				gmove(r->type, r->type, D_TOS, r, rg, r);
				gopcode(o, n->type, rg, r, lg, l);
				regfree(rg);
			} else
				gopcode(o, n->type, D_TOS, r, lg, l);
			gmove(n->type, nn->type, lg, l, result, nn);
			regfree(lg);
			break;
		}
		if(l->complex >= r->complex) {
			if(l->op == OADDR && (o == OADD || o == OSUB))
				lg = regaddr(result);
			else
				lg = regalloc(l->type, result);
			cgen(l, lg, l);
			rg = eval(r, D_NONE);
		} else {
			rg = regalloc(r->type, D_NONE);
			cgen(r, rg, r);
			lg = regalloc(l->type, result);
			cgen(l, lg, l);
		}
		if(o == OXOR) {
			if(rg == D_TREE) {
				rg = regalloc(r->type, D_NONE);
				cgen(r, rg, r);
			}
			if(rg == D_TOS) {
				rg = regalloc(r->type, D_NONE);
				gmove(r->type, r->type, D_TOS, r, rg, r);
			}
		}
		gopcode(o, n->type, rg, r, lg, l);
		gmove(n->type, nn->type, lg, l, result, nn);
		regfree(lg);
		regfree(rg);
		break;

	case OASHL:
		if(r->op == OCONST)
			if(shlcon(l, r, result, nn))
				break;
	case OLSHR:
	case OASHR:
		if(result == D_NONE) {
			nullwarn(l, r);
			break;
		}

		if(l->complex >= FNX && r->complex >= FNX) {
			cgen(r, D_TOS, r);
			v = argoff;
			lg = regalloc(l->type, result);
			cgen(l, lg, l);
			adjsp(v - argoff);
			rg = regalloc(r->type, D_NONE);
			gopcode(OAS, r->type, D_TOS, r, rg, r);
			gopcode(n->op, n->type, rg, r, lg, l);
			gmove(n->type, nn->type, lg, l, result, nn);
			regfree(lg);
			regfree(rg);
			break;
		}
		if(l->complex >= r->complex) {
			lg = regalloc(l->type, result);
			cgen(l, lg, l);
			v = vconst(r);
			if(v <= 0 || v > 8) {
				rg = regalloc(r->type, D_NONE);
				cgen(r, rg, r);
			} else
				rg = eval(r, D_NONE);
		} else {
			rg = regalloc(r->type, D_NONE);
			cgen(r, rg, r);
			lg = regalloc(l->type, result);
			cgen(l, lg, l);
		}
		gopcode(o, n->type, rg, r, lg, l);
		gmove(n->type, nn->type, lg, l, result, nn);
		regfree(lg);
		regfree(rg);
		break;

	case ONEG:
	case OCOM:
		if(result == D_NONE) {
			nullwarn(l, Z);
			break;
		}
		lg = regalloc(l->type, result);
		cgen(l, lg, l);
		gopcode(o, l->type, D_NONE, Z, lg, l);
		gmove(n->type, nn->type, lg, l, result, nn);
		regfree(lg);
		break;

	case OADDR:
		if(result == D_NONE) {
			nullwarn(l, Z);
			break;
		}
		if(l->op == OINDEX && l->scale == 4 && result != D_TOS) {
			/* index scaled by 1, add is better */
			nod = *l;
			nod.op = OADD;
			nod.addable = 0;
			cgen(&nod, result, nn);
			break;
		}
		lcgen(l, result, nn);
		break;

	case OEQ:
	case ONE:
	case OLE:
	case OLT:
	case OGE:
	case OGT:
	case OLO:
	case OLS:
	case OHI:
	case OHS:
		if(result == D_NONE) {
			nullwarn(l, r);
			break;
		}
		boolgen(n, 1, result, nn, Z);
		break;

	case OANDAND:
	case OOROR:
		boolgen(n, 1, result, nn, Z);
		if(result == D_NONE)
			patch(p, pc);
		break;

	case OCOMMA:
		cgen(l, D_NONE, l);
		doinc(l, POST);
		doinc(r, PRE);
		cgen(r, result, nn);
		break;

	case ONOT:
		if(result == D_NONE) {
			nullwarn(l, Z);
			break;
		}
		boolgen(n, 1, result, nn, Z);
		break;

	case OPOSTINC:
	case OPOSTDEC:
		v = 1;
		if(l->type->etype == TIND)
			v = l->type->link->width;
		if(o == OPOSTDEC)
			v = -v;
		if(l->op == OBIT)
			goto bitinc;
		if(nn == Z)
			goto pre;

		lg = D_TREE;
		if(l->addable < INDEXED) {
			lg = regaddr(D_NONE);
			lcgen(l, lg, Z);
			lg |= I_INDIR;
		}
		if(result != D_NONE)
			gmove(l->type, nn->type, lg, l, result, nn);
		if(typefd[n->type->etype]) {
			rg = regalloc(n->type, D_NONE);
			gmove(l->type, l->type, lg, l, rg, l);
			gopcode(o, n->type, D_CONST, nodconst(1), rg, l);
			gmove(l->type, l->type, rg, l, lg, l);
			regfree(rg);
		} else {
			if(v < 0)
				gopcode(o, n->type, D_CONST, nodconst(-v), lg, l);
			else
				gopcode(o, n->type, D_CONST, nodconst(v), lg, l);
		}
		regfree(lg);
		break;

	case OPREINC:
	case OPREDEC:
		v = 1;
		if(l->type->etype == TIND)
			v = l->type->link->width;
		if(o == OPREDEC)
			v = -v;
		if(l->op == OBIT)
			goto bitinc;

	pre:
		lg = D_TREE;
		if(l->addable < INDEXED) {
			lg = regaddr(D_NONE);
			lcgen(l, lg, Z);
			lg |= I_INDIR;
		}
		if(typefd[n->type->etype]) {
			rg = regalloc(n->type, D_NONE);
			gmove(l->type, l->type, lg, l, rg, l);
			gopcode(o, n->type, D_CONST, nodconst(1), rg, l);
			gmove(l->type, l->type, rg, l, lg, l);
			regfree(rg);
		} else {
			if(v < 0)
				gopcode(o, n->type, D_CONST, nodconst(-v), lg, l);
			else
				gopcode(o, n->type, D_CONST, nodconst(v), lg, l);
		}
		if(result != D_NONE)
			gmove(l->type, nn->type, lg, l, result, nn);
		regfree(lg);
		break;

	bitinc:
		rg = regaddr(D_NONE);
		lg = regalloc(tfield, D_NONE);
		if(result != D_NONE && (o == OPOSTINC || o == OPOSTDEC)) {
			g = bitload(l, lg, rg, D_NONE, nn);
			if(nn != Z)
				gmove(l->type, nn->type, g, l, result, nn);
			if(v < 0)
				gopcode(o, n->type, D_CONST, nodconst(-v), g, n);
			else
				gopcode(o, n->type, D_CONST, nodconst(v), g, n);
			bitstore(l, g, rg, lg, D_NONE, nn);
			break;
		}
		g = bitload(l, lg, rg, result, nn);
		if(v < 0)
			gopcode(o, n->type, D_CONST, nodconst(-v), g, n);
		else
			gopcode(o, n->type, D_CONST, nodconst(v), g, n);
		if(result != D_NONE)
			gmove(l->type, nn->type, g, l, result, nn);
		bitstore(l, g, rg, lg, D_NONE, nn);
		break;
	}
}

void
lcgen(Node *n, int result, Node *nn)
{
	Node rn;
	Prog *p1;
	int lg;

	if(n == Z || n->type == T)
		return;
	if(debug['g']) {
		if(result == D_TREE)
			prtree(nn, "result");
		else
			print("result = %R\n", result);
		prtree(n, "lcgen");
	}
	if(nn == Z) {
		nn = &rn;
		nn->type = types[TIND];
	}
	switch(n->op) {
	case OCOMMA:
		cgen(n->left, D_NONE, n->left);
		doinc(n->left, POST);
		doinc(n->right, PRE);
		lcgen(n->right, result, nn);
		break;

	case OCOND:
		doinc(n->left, PRE);
		boolgen(n->left, 1, D_NONE, Z, n->left);
		p1 = p;

		inargs++;
		doinc(n->right->left, PRE);
		lcgen(n->right->left, result, nn);
		doinc(n->right->left, POST);
		gbranch(OGOTO);
		patch(p1, pc);
		p1 = p;

		doinc(n->right->right, PRE);
		lcgen(n->right->right, result, nn);
		doinc(n->right->right, POST);
		patch(p1, pc);
		inargs--;
		break;

	case OIND:
		if(n->addable >= INDEXED) {
			if(result >= D_A0 && result < D_A0+NREG) {
				gopcode(OADDR, types[TLONG], D_TREE, n, result, nn);
				break;
			}
			if(result == D_TOS) {
				gopcode(OADDR, types[TSHORT], D_NONE, nn, D_TREE, n);
				break;
			}
		}
		cgen(n->left, result, nn);
		break;

	default:
		if(n->addable < INDEXED) {
			diag(n, "unknown op in lcgen: %O", n->op);
			break;
		}
		if(result >= D_A0 && result < D_A0+NREG) {
			gopcode(OADDR, types[TLONG], D_TREE, n, result, nn);
			break;
		}
		if(result == D_TOS) {
			gopcode(OADDR, types[TSHORT], D_NONE, nn, D_TREE, n);
			break;
		}
		lg = regaddr(result);
		gopcode(OADDR, types[TLONG], D_TREE, n, lg, nn);
		gopcode(OAS, nn->type, lg, nn, result, nn);
		regfree(lg);
		break;
	}
}

void
bcgen(Node *n, int true)
{

	boolgen(n, true, D_NONE, Z, Z);
}

void
boolgen(Node *n, int true, int result, Node *nn, Node *post)
{
	Prog *p1, *p2;
	Node *l, *r;
	int lg, rg, fp, o;
	long v;

	if(debug['g']) {
		if(result == D_TREE)
			prtree(nn, "result");
		else
			print("result = %R\n", result);
		prtree(n, "boolgen");
	}
	l = n->left;
	r = n->right;
	switch(n->op) {

	default:
		lg = eval(n, result);
		if(lg >= D_A0 && lg < D_A0+NREG) {
			rg = regalloc(types[TLONG], D_NONE);
			gopcode(OAS, types[TLONG], lg, n, rg, Z);
			regfree(rg);
		} else
			gopcode(OTST, n->type, D_NONE, Z, lg, n);
		regfree(lg);
		o = ONE;
		fp = typefd[n->type->etype];
		goto genbool;

	case OCONST:
		fp = vconst(n);
		if(!true)
			fp = !fp;
		gbranch(OGOTO);
		if(fp) {
			p1 = p;
			gbranch(OGOTO);
			patch(p1, pc);
		}
		goto com;

	case ONOT:
		boolgen(l, !true, result, nn, post);
		break;

	case OCOND:
		doinc(l, PRE);
		boolgen(l, 1, D_NONE, Z, l);
		p1 = p;

		inargs++;
		doinc(r->left, PRE);
		boolgen(r->left, true, result, nn, r->left);
		if(result != D_NONE) {
			doinc(r->left, POST);
			gbranch(OGOTO);
			patch(p1, pc);
			p1 = p;

			doinc(r->right, PRE);
			boolgen(r->right, !true, result, nn, r->right);
			doinc(r->right, POST);
			patch(p1, pc);
			inargs--;
			break;
		}
		p2 = p;
		gbranch(OGOTO);
		patch(p1, pc);
		p1 = p;

		doinc(r->right, PRE);
		boolgen(r->right, !true, result, nn, r->right);
		patch(p2, pc);
		p2 = p;
		if(doinc(post, POST|TEST)) {
			lg = regalloc(types[TSHORT], D_NONE);
			gopcode(OAS, types[TSHORT], D_CCR, Z, lg, Z);
			doinc(post, POST);
			gopcode(OAS, types[TSHORT], lg, Z, D_CCR, Z);
			regfree(lg);
		}
		gbranch(OGOTO);
		patch(p1, pc);
		patch(p2, pc);
		inargs--;
		goto com;

	case OANDAND:
		if(!true)
			goto caseor;

	caseand:
		doinc(l, PRE);
		boolgen(l, true, D_NONE, Z, l);
		p1 = p;
		inargs++;
		doinc(r, PRE);
		boolgen(r, !true, D_NONE, Z, r);
		p2 = p;
		patch(p1, pc);
		gbranch(OGOTO);
		patch(p2, pc);
		inargs--;
		goto com;

	case OOROR:
		if(!true)
			goto caseand;

	caseor:
		doinc(l, PRE);
		boolgen(l, !true, D_NONE, Z, l);
		p1 = p;
		inargs++;
		doinc(r, PRE);
		boolgen(r, !true, D_NONE, Z, r);
		p2 = p;
		gbranch(OGOTO);
		patch(p1, pc);
		patch(p2, pc);
		inargs--;
		goto com;

	case OEQ:
	case ONE:
		if(vconst(l) == 0) {
			if(n->op == ONE) {
				boolgen(r, true, result, nn, post);
				break;
			}
			boolgen(r, !true, result, nn, post);
			break;
		}

	case OLE:
	case OLT:
	case OGE:
	case OGT:
	case OHI:
	case OHS:
	case OLO:
	case OLS:
		fp = typefd[r->type->etype];
		if(l->op == OCONST) {
			v = vconst(l);
			if(v == 0) {	/* tst instruction */
				o = invrel[relindex(n->op)];
				rg = eval(r, result);
				gopcode(OTST, r->type, D_NONE, Z, rg, r);
				regfree(rg);
				goto genbool;
			}
			if(!fp) {	/* cmpi and movq, saves about .5% both time and space */
				if(v < 128 && v >= -128 &&
				   ewidth[r->type->etype] == SZ_LONG) {
					rg = eval(r, result);
					lg = regalloc(l->type, D_NONE);
					cgen(l, lg, l);
					o = n->op;
					gopcode(o, l->type, lg, l, rg, r);
					regfree(lg);
					regfree(rg);
					goto genbool;
				}
				o = invrel[relindex(n->op)];
				rg = eval(r, result);
				gopcode(o, r->type, rg, r, D_TREE, l);
				regfree(rg);
				goto genbool;
			}
		}
		lg = D_TOS;
		if(r->complex < FNX)
			lg = regalloc(l->type, lg);
		cgen(l, lg, l);
		v = argoff;
		rg = eval(r, result);
		if(lg == D_TOS) {
			adjsp(v - argoff);
			lg = regalloc(l->type, lg);
			gopcode(OAS, l->type, D_TOS, l, lg, l);
		}
		o = n->op;
		gopcode(o, l->type, lg, l, rg, r);
		regfree(lg);
		regfree(rg);

	genbool:
		if(true)
			o = comrel[relindex(o)];
		if(doinc(post, POST|TEST)) {
			lg = regalloc(types[TSHORT], D_NONE);
			gopcode(OAS, types[TSHORT], D_CCR, Z, lg, Z);
			doinc(post, POST);
			gopcode(OAS, types[TSHORT], lg, Z, D_CCR, Z);
			regfree(lg);
		}
		gbranch(o);
		if(fp)
			fpbranch();

	com:
		if(result == D_NONE)
			break;
		p1 = p;
		gopcode(OAS, nn->type, D_CONST, nodconst(1), result, nn);
		gbranch(OGOTO);
		p2 = p;
		patch(p1, pc);
		gopcode(OAS, nn->type, D_CONST, nodconst(0), result, nn);
		patch(p2, pc);
		break;
	}
}

void
sugen(Node *n, int result, Node *nn, long w)
{
	long s, v, o;
	int lg, rg, ng;
	Prog *p1;
	Node *l, *r, nod;
	Type *t;

	if(n == Z || n->type == T)
		return;
	if(debug['g']) {
		if(result == D_TREE)
			prtree(nn, "result");
		else
			print("result = %R width = %ld\n", result, w);
		prtree(n, "sugen");
	}
	s = argoff;
	if(result == D_TREE) {
		if(nn == nodrat)
			if(w > nrathole)
				nrathole = w;
	}

	if(n->addable >= INDEXED && n->op != OCONST)
		goto copy;
	switch(n->op) {
	default:
		diag(n, "unknown op in sugen: %O", n->op);
		break;

	case OCONST:
		if(n->type && typev[n->type->etype]) {
			if(result == D_NONE) {
				nullwarn(n->left, Z);
				break;
			}

			lg = regaddr(D_NONE);
			if(result == D_TOS) {
				adjsp(s - argoff + w);
				gopcode(OADDR, types[TIND], result, nn, lg, n);
			} else
			if(result == D_TREE) {
				lcgen(nn, lg, Z);
			} else
				diag(n, "unknown su result: %R", result);

			gopcode(OAS, types[TLONG], D_CONST, nodconst((long)(n->vconst>>32)),
				lg|I_INDINC, n);
			gopcode(OAS, types[TLONG], D_CONST, nodconst((long)(n->vconst)),
				lg|I_INDINC, n);
			regfree(lg);
			break;
		}
		goto copy;

	case ODOT:
		l = n->left;
		sugen(l, D_TREE, nodrat, l->type->width);
		if(result != D_NONE) {
			warn(n, "non-interruptable temporary");
			nod = *nodrat;
			r = n->right;
			if(!r || r->op != OCONST) {
				diag(n, "DOT and no offset");
				break;
			}
			nod.xoffset += r->vconst;
			nod.type = n->type;
			sugen(&nod, result, nn, w);
		}
		break;

	case OIND:
		if(result == D_NONE) {
			nullwarn(n->left, Z);
			break;
		}
		goto copy;

	case OSTRUCT:
		lg = nodalloc(types[TIND], result, &nod);
		nod.lineno = n->lineno;
		if(result == D_TREE)
			lcgen(nn, lg, Z);
		else
		if(result == D_TOS) {
			adjsp(s - argoff + w);
			gopcode(OADDR, types[TIND], result, nn, lg, n);
		} else
			diag(n, "unknown su result: %R", result);
		o = 0;
		r = n->left;
		for(t = n->type->link; t != T; t = t->down) {
			l = r;			
			if(r->op == OLIST) {
				l = r->left;
				r = r->right;
			}
			nod.type = t;
			if(l->complex < FNX) {
				nod.xoffset = 0;
				if(o != t->offset) {
					gopcode(OADD, types[TIND], D_CONST,
						nodconst(t->offset-o), lg, Z);
					o = t->offset;
				}
				cgen(l, D_TREE, &nod);
				continue;
			}
			nod.xoffset = t->offset - o;
			gopcode(OAS, types[TIND], lg, Z, D_TOS, Z);
			s = argoff;
			if(typesuv[t->etype]) {
				sugen(l, D_TREE, nodrat, t->width);
				adjsp(s - argoff);
				gopcode(OAS, types[TIND], D_TOS, Z, lg, Z);
				warn(n, "non-interruptable temporary");
				sugen(nodrat, D_TREE, &nod, t->width);
				continue;
			}
			rg = regalloc(t, D_NONE);
			cgen(l, rg, l);
			adjsp(s - argoff);
			gopcode(OAS, types[TIND], D_TOS, Z, lg, Z);
			gopcode(OAS, t, rg, Z, D_TREE, &nod);
			regfree(rg);
		}
		regfree(lg);
		break;

	case OAS:
		if(result == D_NONE) {
			sugen(n->right, D_TREE, n->left, w);
			break;
		}
		sugen(n->right, D_TREE, nodrat, w);	/* could do better */
		warn(n, "non-interruptable temporary");
		sugen(nodrat, D_TREE, n->left, w);
		sugen(nodrat, result, nn, w);
		break;

	case OFUNC:
		if(result == D_NONE) {
			sugen(n, D_TREE, nodrat, w);
			break;
		}
		inargs++;
		/* prepare zero-th arg: address of result */
		if(result == D_TOS) {
			adjsp(s - argoff + w);
			v = argoff;
			gargs(n->right);
			gopcode(OADDR, types[TSHORT], D_NONE, nn, result, nn);
			p->to.type = D_STACK;
			p->to.offset = argoff - v;
		} else
		if(result == D_TREE) {
			v = argoff;
			gargs(n->right);
			if(nn->complex >= FNX) {
				rg = regalloc(types[TIND], regret(types[TIND]));
				lcgen(nn, rg, Z);
				gopcode(OAS, types[TIND], rg, Z, D_TOS, Z);
				regfree(rg);
			} else
				lcgen(nn, D_TOS, Z);
		} else {
			diag(n, "unknown result in FUNC sugen");
			break;
		}
		argoff += types[TIND]->width;
		l = n->left;
		lg = D_TREE;
		if(l->addable < INDEXED) {
			lg = regaddr(result);
			lcgen(l, lg, Z);
			lg |= I_INDIR;
		}
		inargs--;
		doinc(n->right, POST);
		doinc(n->left, POST);
		gopcode(OFUNC, types[TCHAR], D_NONE, Z, lg, l);
		regfree(lg);
		if(inargs)
			adjsp(v - argoff);
		break;

	case OCOND:
		doinc(n->left, PRE);
		boolgen(n->left, 1, D_NONE, Z, n->left);
		p1 = p;

		inargs++;
		doinc(n->right->left, PRE);
		sugen(n->right->left, result, nn, w);
		doinc(n->right->left, POST);
		gbranch(OGOTO);
		patch(p1, pc);
		p1 = p;

		doinc(n->right->right, PRE);
		sugen(n->right->right, result, nn, w);
		doinc(n->right->right, POST);
		patch(p1, pc);
		inargs--;
		break;

	case OCOMMA:
		cgen(n->left, D_NONE, n->left);
		doinc(n->left, POST);
		doinc(n->right, PRE);
		sugen(n->right, result, nn, w);
		break;
	}
	return;

copy:
	if(result == D_NONE)
		return;
	rg = regaddr(D_NONE);
	lcgen(n, rg, Z);

	lg = regaddr(D_NONE);
	if(result == D_TOS) {
		adjsp(s - argoff + w);
		gopcode(OADDR, types[TIND], result, nn, lg, n);
	} else
	if(result == D_TREE) {
		if(nn->complex >= FNX) {
			gopcode(OAS, types[TIND], rg, n, D_TOS, n);
			s = argoff;
			lcgen(nn, lg, Z);
			adjsp(s - argoff);
			gopcode(OAS, types[TIND], D_TOS, n, rg, n);
		} else
			lcgen(nn, lg, Z);
	} else
		diag(n, "unknown su result: %R", result);

	if(w % SZ_LONG)
		diag(Z, "sucopy width not 0%%%d", SZ_LONG);
	v = w / SZ_LONG;
	if(v & 1) {
		gopcode(OAS, types[TLONG], rg|I_INDINC, n, lg|I_INDINC, n);
		v--;
	}
	if(v > 6) {
		ng = regalloc(types[TLONG], D_NONE);
		gopcode(OAS, types[TLONG], D_CONST, nodconst(v/2-1), ng, n);
		v = pc;
		gopcode(OAS, types[TLONG], rg|I_INDINC, n, lg|I_INDINC, n);
		gopcode(OAS, types[TLONG], rg|I_INDINC, n, lg|I_INDINC, n);
		gbranch(OGT);
		patch(p, v);
		p->from.type = ng;
		p->as = ADBF;
		regfree(ng);
	} else
		while(v > 0) {
			gopcode(OAS, types[TLONG], rg|I_INDINC, n, lg|I_INDINC, n);
			v--;
		}

	regfree(lg);
	regfree(rg);
}
