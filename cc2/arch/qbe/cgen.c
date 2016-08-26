/* See LICENSE file for copyright and license details. */
#include <assert.h>
#include <stdlib.h>

#include "arch.h"
#include "../../cc2.h"
#include "../../../inc/sizes.h"

enum sflags {
	ISTMP  = 1,
	ISCONS = 2
};

static char opasmw[] = {
	[OADD] = ASADDW,
	[OSUB] = ASSUBW,
	[OMUL] = ASMULW,
	[OMOD] = ASMODW,
	[ODIV] = ASDIVW,
	[OSHL] = ASSHLW,
	[OSHR] = ASSHRW,
	[OLT] = ASLTW,
	[OGT] = ASGTW,
	[OLE] = ASLEW,
	[OGE] = ASGEW,
	[OEQ] = ASEQW,
	[ONE] = ASNEW,
	[OBAND] = ASBANDW,
	[OBOR] = ASBORW,
	[OBXOR] = ASBXORW,
	[OCPL] = ASCPLW
};

static char opasml[] = {
	[OADD] = ASADDL,
	[OSUB] = ASSUBL,
	[OMUL] = ASMULL,
	[OMOD] = ASMODL,
	[ODIV] = ASDIVL,
	[OSHL] = ASSHLL,
	[OSHR] = ASSHRL,
	[OLT] = ASLTL,
	[OGT] = ASGTL,
	[OLE] = ASLEL,
	[OGE] = ASGEL,
	[OEQ] = ASEQL,
	[ONE] = ASNEL,
	[OBAND] = ASBANDL,
	[OBOR] = ASBORL,
	[OBXOR] = ASBXORL,
	[OCPL] = ASCPLL
};

static char opasms[] = {
	[OADD] = ASADDS,
	[OSUB] = ASSUBS,
	[OMUL] = ASMULS,
	[OMOD] = ASMODS,
	[ODIV] = ASDIVS,
	[OSHL] = ASSHLS,
	[OSHR] = ASSHRS,
	[OLT] = ASLTS,
	[OGT] = ASGTS,
	[OLE] = ASLES,
	[OGE] = ASGES,
	[OEQ] = ASEQS,
	[ONE] = ASNES,
	[OBAND] = ASBANDS,
	[OBOR] = ASBORS,
	[OBXOR] = ASBXORS,
	[OCPL] = ASCPLS
};
static char opasmd[] = {
	[OADD] = ASADDD,
	[OSUB] = ASSUBD,
	[OMUL] = ASMULD,
	[OMOD] = ASMODD,
	[ODIV] = ASDIVD,
	[OSHL] = ASSHLD,
	[OSHR] = ASSHRD,
	[OLT] = ASLTD,
	[OGT] = ASGTD,
	[OLE] = ASLED,
	[OGE] = ASGED,
	[OEQ] = ASEQD,
	[ONE] = ASNED,
	[OBAND] = ASBANDD,
	[OBOR] = ASBORD,
	[OBXOR] = ASBXORD,
	[OCPL] = ASCPLD
};

extern Type int32type, uint32type;

static Node *
tmpnode(Node *np, Type *tp)
{
	Symbol *sym;

	if (!np)
		np = newnode(OTMP);
	sym = getsym(TMPSYM);
	sym->type = np->type = *tp;
	sym->kind = STMP;
	np->u.sym = sym;
	np->op = OTMP;
	np->flags |= ISTMP;
	return np;
}

static Node *
load(Node *np, Node *new)
{
	int op;
	Type *tp;

	tp = &np->type;
	switch (tp->size) {
	case 1:
		op = ASLDB;
		break;
	case 2:
		op = ASLDH;
		break;
	case 4:
		op = (tp->flags & INTF) ? ASLDW : ASLDS;
		break;
	case 8:
		op = (tp->flags & INTF) ? ASLDL : ASLDD;
		break;
	default:
		abort();
	}
	code(op, tmpnode(new, tp), np, NULL);

	return new;
}

static Node *rhs(Node *np, Node *new);

static Node *
cast(Type *td, Node *ns, Node *nd)
{
	Type *ts;
	Node aux1, aux2;
	int op, d_isint, s_isint;

	ts = &ns->type;
	d_isint = (td->flags & INTF) != 0;
	s_isint = (ts->flags & INTF) != 0;

	if (d_isint && s_isint) {
		if (td->size <= ts->size)
			return nd;
		assert(td->size == 4 || td->size == 8);
		switch (ts->size) {
		case 1:
			op = (td->size == 4) ? ASEXTBW : ASEXTBL;
			break;
		case 2:
			op = (td->size == 4) ? ASEXTHW : ASEXTHL;
			break;
		case 4:
			op = ASEXTWL;
			break;
		default:
			abort();
		}
		/*
		 * unsigned version of operations are always +1 the
		 * signed version
		 */
		op += (td->flags & SIGNF) == 0;
	} else if (d_isint) {
		/* conversion from float to int */
		switch (ts->size) {
		case 4:
			op = (td->size == 8) ? ASSTOL : ASSTOW;
			break;
		case 8:
			op = (td->size == 8) ? ASDTOL : ASDTOW;
			break;
		default:
			abort();
		}
		/* TODO: Add signess */
	} else if (s_isint) {
		/* conversion from int to float */
		switch (ts->size) {
		case 1:
		case 2:
			ts = (ts->flags&SIGNF) ? &int32type : &uint32type;
			ns = cast(ts, ns, tmpnode(&aux2, ts));
		case 4:
			op = (td->size == 8) ? ASSWTOD : ASSWTOS;
			break;
		case 8:
			op = (td->size == 8) ? ASSLTOD : ASSLTOS;
			break;
		default:
			abort();
		}
		/* TODO: Add signess */
	} else {
		/* conversion from float to float */
		op = (td->size == 4) ? ASEXTS : ASTRUNCD;
	}

	code(op, tmpnode(nd, td), ns, NULL);
	return nd;
}

static Node *rhs(Node *np, Node *new);

static Node *
call(Node *np, Node *ret)
{
	int n, op;
	Type *tp;
	Node aux, **q, *p, *pars[NR_FUNPARAM];

	for (n = 0, p = np->right; p; p = p->right)
		pars[n++] = rhs(p->left, newnode(OTMP));

	tp = &np->type;
	switch (tp->size) {
	case 0:
		np->left = tmpnode(NULL, tp);
		op = ASCALLW;
		break;
	case 1:
		op = ASCALLB;
		break;
	case 2:
		op = ASCALLH;
		break;
	case 4:
		op = (tp->flags & INTF) ? ASCALLW : ASCALLS;
		break;
	case 8:
		op = (tp->flags & INTF) ? ASCALLL : ASCALLD;
		break;
	default:
		abort();
	}
	code(op, tmpnode(ret, tp), np->left, NULL);

	for (q = pars; q < &pars[n]; ++q) {
		op = (q == &pars[n-1]) ? ASPARE : ASPAR;
		tmpnode(&aux, &(*q)->type);
		code(op, NULL, *q, &aux);
	}
	code(ASCALL, NULL, NULL, NULL);

	return ret;
}

static Node *
abbrev(Node *np, Node *ret)
{
	Node aux;

	tmpnode(&aux, &np->type);
	aux.right = np->right;
	aux.left = np->left;
	return rhs(&aux, ret);
}

static Node *
assign(Node *to, Node *from)
{
	Type *tp;
	int op;

	tp = &to->type;
	switch (tp->size) {
	case 1:
		op = ASSTB;
		break;
	case 2:
		op = ASSTH;
		break;
	case 4:
		op = (tp->flags & INTF) ? ASSTW : ASSTS;
		break;
	case 8:
		op = (tp->flags & INTF) ? ASSTL : ASSTD;
		break;
	default:
		abort();
	}
	code(op, to, from, NULL);
	return from;
}

static Node *
lhs(Node *np, Node *new)
{
	switch (np->op) {
	case OMEM:
	case OAUTO:
		*new = *np;
		return np;
	case OPTR:
		return rhs(np->left, new);
	default:
		abort();
	}
}

static void
bool(Node *np, Symbol *true, Symbol *false)
{
	Node *l = np->left, *r = np->right;
	Node ret, ifyes, ifno;
	Symbol *label;

	switch (np->op) {
	case ONEG:
		bool(l, false, true);
		break;
	case OAND:
		label = newlabel();
		bool(l, label, false);
		setlabel(label);
		bool(r, true, false);
		break;
	case OOR:
		label = newlabel();
		bool(l, true, label);
		setlabel(label);
		bool(r, true, false);
		break;
	default:
		label2node(&ifyes, true);
		label2node(&ifno, false);
		code(ASBRANCH, rhs(l, &ret), &ifyes, &ifno);
		break;
	}
}

static Node *
ternary(Node *np, Node *ret)
{
	Node ifyes, ifno, phi, *colon, aux1, aux2, aux3;

	tmpnode(ret, &np->type);
	label2node(&ifyes, NULL);
	label2node(&ifno, NULL);
	label2node(&phi, NULL);

	colon = np->right;
	code(ASBRANCH, rhs(np->left, &aux1), &ifyes, &ifno);

	setlabel(ifyes.u.sym);
	assign(ret, rhs(colon->left, &aux2));
	code(ASJMP, NULL, &phi, NULL);

	setlabel(ifno.u.sym);
	assign(ret, rhs(colon->right, &aux3));
	setlabel(phi.u.sym);

	return ret;
}

static Node *
function(void)
{
	Node aux;
	Symbol *p;

	/* allocate stack space for parameters */
	for (p = locals; p && (p->type.flags & PARF) != 0; p = p->next)
		code(ASALLOC, label2node(&aux, p), NULL, NULL);

	/* allocate stack space for local variables) */
	for ( ; p && p->id != TMPSYM; p = p->next) {
		if (p->kind != SAUTO)
			continue;
		code(ASALLOC, label2node(&aux, p), NULL, NULL);
	}
	/* store formal parameters in parameters */
	for (p = locals; p; p = p->next) {
		if ((p->type.flags & PARF) == 0)
			break;
		code(ASFORM, label2node(&aux, p), NULL, NULL);
	}
	return NULL;
}

static Node *
rhs(Node *np, Node *ret)
{
	Node aux1, aux2, *phi, *l = np->left, *r = np->right;
	Type *tp;
	int off, op;
	char *tbl;
	Symbol *true, *false;

	tp = &np->type;

	switch (np->op) {
	case OBFUN:
		return function();
	case ONOP:
	case OBLOOP:
	case OELOOP:
	case OEFUN:
		return NULL;
	case OCONST:
		*ret = *np;
		return np;
	case OMEM:
	case OAUTO:
		return load(np, ret);
	case ONEG:
	case OAND:
	case OOR:
		true = newlabel();
		false = newlabel();
		phi = label2node(&aux1, NULL);
		tmpnode(ret, &int32type);

		bool(np, true, false);

		setlabel(true);
		assign(ret, constnode(1, &int32type));
		code(ASJMP, NULL, phi, NULL);

		setlabel(false);
		assign(ret, constnode(0, &int32type));

		setlabel(phi->u.sym);
		return ret;
        case OSHR:
        case OMOD:
        case ODIV:
        case OLT:
        case OGT:
        case OLE:
        case OGE:
                /*
                 * unsigned version of operations are always +1 the
                 * signed version
                 */
                off = (tp->flags & SIGNF) == 0;
                goto binary;
        case OADD:
        case OSUB:
        case OMUL:
        case OSHL:
        case OBAND:
        case OBOR:
        case OBXOR:
        case OEQ:
        case ONE:
                off = 0;
        binary:
		if (l->complex >= r->complex) {
			rhs(l, &aux1);
			rhs(r, &aux2);
		} else {
			rhs(r, &aux2);
			rhs(l, &aux1);
		}
                switch (tp->size) {
                case 4:
                        tbl = (tp->flags & INTF) ? opasmw : opasms;
                        break;
                case 8:
                        tbl = (tp->flags & INTF) ? opasml : opasmd;
                        break;
                default:
                        abort();
                }
                op = tbl[np->op] + off;
		tmpnode(ret, tp);
                code(op, ret, &aux1, &aux2);
                return ret;
	case OCALL:
		if (np->left->op == OPTR)
			np = rhs(l, &aux1);
		return call(np, ret);
	case OCAST:
		return cast(tp, rhs(l, &aux1), ret);
	case OASSIG:
		/* TODO: see what is the more difficult */
		if (np->u.subop != 0)
			r = abbrev(np, &aux1);
		lhs(l, &aux2);
		rhs(r, ret);
		return assign(&aux2, ret);
	case OASK:
		return ternary(np, ret);
	case OCOMMA:
		return rhs(np, ret);
	case OPTR:
		return load(rhs(l, &aux1), ret);
	case OADDR:
		return lhs(l, ret);
	case OFIELD:
	case OINC:
	case ODEC:
	case OCASE:
	case ODEFAULT:
	case OESWITCH:
	case OBSWITCH:
		/* TODO: implement these operators */
	default:
		abort();
	}
	abort();
}

Node *
cgen(Node *np)
{
	Node ret, aux1, aux2, *p, *next, ifyes, ifno;

	setlabel(np->label);
	switch (np->op) {
	case OJMP:
		label2node(&ifyes, np->u.sym);
		code(ASJMP, NULL, &ifyes, NULL);
		break;
        case OBRANCH:
                next = np->next;
                if (!next->label)
                        next->label = newlabel();

                label2node(&ifyes, np->u.sym);
                label2node(&ifno, next->label);
		rhs(np->left, &ret);
		code(ASBRANCH, &ret, &ifyes, &ifno);
                break;
	case ORET:
		p = (np->left) ? rhs(np->left, &ret) : NULL;
		code(ASRET, NULL, p, NULL);
		break;
	default:
		rhs(np, &ret);
		break;
	}
	return NULL;
}

/*
 * This is strongly influenced by
 * http://plan9.bell-labs.com/sys/doc/compiler.ps (/sys/doc/compiler.ps)
 * calculate addresability as follows
 *     AUTO => 11          value+fp
 *     REG => 11           reg
 *     STATIC => 11        (value)
 *     CONST => 11         $value
 * These values of addressability are not used in the code generation.
 * They are only used to calculate the Sethi-Ullman numbers. Since
 * QBE is AMD64 targered we could do a better job there, and try to
 * detect some of the complex addressing modes of these processors.
 */
Node *
sethi(Node *np)
{
	Node *lp, *rp;

	if (!np)
		return np;

	np->complex = 0;
	np->address = 0;
	lp = np->left;
	rp = np->right;

	switch (np->op) {
	case OAUTO:
	case OREG:
	case OMEM:
	case OCONST:
		np->address = 11;
		break;
	case OCPL:
		np->op = OAND;
		rp = constnode(~(TUINT) 0, &np->type);
		goto binary;
	case OSNEG:
		np->op = OSUB;
		rp = lp;
		lp = constnode(0, &np->type);
		if ((np->type.flags & INTF) == 0)
			lp->u.f = 0.0;
	default:
	binary:
		lp = sethi(lp);
		rp = sethi(rp);
		break;
	}
	np->left = lp;
	np->right = rp;

	if (np->address > 10)
		return np;
	if (lp)
		np->complex = lp->complex;
	if (rp) {
		int d = np->complex - rp->complex;

		if (d == 0)
			++np->complex;
		else if (d < 0)
			np->complex = rp->complex;
	}
	if (np->complex == 0)
		++np->complex;
	return np;
}
