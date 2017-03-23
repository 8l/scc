/* See LICENSE file for copyright and license details. */
static char sccsid[] = "@(#) ./cc2/arch/qbe/cgen.c";

#include <assert.h>
#include <stdlib.h>

#include <cstd.h>
#include "arch.h"
#include "../../../inc/cc.h"
#include "../../cc2.h"

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
};

static char opasms[] = {
	[OADD] = ASADDS,
	[OSUB] = ASSUBS,
	[OMUL] = ASMULS,
	[ODIV] = ASDIVS,
	[OLT] = ASLTS,
	[OGT] = ASGTS,
	[OLE] = ASLES,
	[OGE] = ASGES,
	[OEQ] = ASEQS,
	[ONE] = ASNES,
};
static char opasmd[] = {
	[OADD] = ASADDD,
	[OSUB] = ASSUBD,
	[OMUL] = ASMULD,
	[ODIV] = ASDIVD,
	[OLT] = ASLTD,
	[OGT] = ASGTD,
	[OLE] = ASLED,
	[OGE] = ASGED,
	[OEQ] = ASEQD,
	[ONE] = ASNED,
};

extern Type int32type, uint32type, ptrtype;

static Node *
tmpnode(Node *np, Type *tp)
{
	char flags;
	Symbol *sym;

	if (!np)
		np = newnode(OTMP);
	sym = getsym(TMPSYM);
	sym->type = np->type = *tp;
	flags = tp->flags & ~(PARF|INITF);
	sym->type.flags = np->type.flags = flags;
	sym->kind = STMP;
	np->left = np->right = NULL;
	np->u.sym = sym;
	np->op = OTMP;
	np->flags |= ISTMP;
	return np;
}

static Node *
load(Type *tp, Node *np, Node *new)
{
	int op;
	int flags = tp->flags;

	if (flags & (AGGRF|FUNF)) {
		*new = *np;
		return new;
	}
	switch (tp->size) {
	case 1:
		op = ASLDSB;
		break;
	case 2:
		op = ASLDSH;
		break;
	case 4:
		op = (flags & FLOATF) ? ASLDS : ASLDSW;
		break;
	case 8:
		op = (flags & FLOATF) ? ASLDD : ASLDL;
		break;
	default:
		abort();
	}
	/*
	 * unsigned version of operations are always +1 the
	 * signed version
	 */
	if ((flags & (INTF|SIGNF)) == INTF && tp->size < 8)
		++op;

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
		if (td->size <= ts->size) {
			*nd = *ns;
			return nd;
		}
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
		op += (ts->flags & SIGNF) == 0;
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
call(Node *np, Node *fun, Node *ret)
{
	int n, op;
	Type *tp;
	Node aux, **q, *p, *pars[NR_FUNPARAM];

	for (n = 0, p = np->right; p; p = p->right)
		pars[n++] = rhs(p->left, newnode(OTMP));

	tp = &np->type;
	code(ASCALL, tmpnode(ret, tp), fun, NULL);

	for (q = pars; q < &pars[n]; ++q) {
		op = (q == &pars[n-1]) ? ASPARE : ASPAR;
		tmpnode(&aux, &(*q)->type);
		code(op, NULL, *q, &aux);
	}
	code((np->op == OCALL) ? ASCALLE : ASCALLEX, NULL, NULL, NULL);

	return ret;
}

static Node *
assign(Type *tp, Node *to, Node *from)
{
	int op;

	switch (tp->size) {
	case 1:
		op = ASSTB;
		break;
	case 2:
		op = ASSTH;
		break;
	case 4:
		op = (tp->flags & FLOATF) ? ASSTS : ASSTW;
		break;
	case 8:
		op = (tp->flags & FLOATF) ? ASSTD : ASSTL;
		break;
	default:
		op = ASSTM;
		break;
	}
	code(op, to, from, NULL);
	return from;
}

static Node *
copy(Type *tp, Node *to, Node *from)
{
	int op;

	switch (tp->size) {
	case 1:
		op = ASCOPYB;
		break;
	case 2:
		op = ASCOPYH;
		break;
	case 4:
		op = (tp->flags & FLOATF) ? ASCOPYS : ASCOPYW;
		break;
	case 8:
		op = (tp->flags & FLOATF) ? ASCOPYD : ASCOPYL;
		break;
	default:
		/* TODO: Need to handle the general case */
		abort();
	}
	code(op, to, from, NULL);
	return from;
}

/* TODO: Do field() transformation in sethi */

static Node *
field(Node *np, Node *ret, int islhs)
{
	Node base, node, off, add, *addr;
	TUINT offset = np->right->u.sym->u.off;

	addr = rhs(np->left, &base);

	if (offset != 0) {
		node.op = OADD;
		node.type = ptrtype;
		node.left = addr;
		node.right = constnode(&off, offset, &ptrtype);
		addr = rhs(&node, &add);
	}

	if (islhs)
		*ret = *addr;
	else
		load(&np->type, addr, ret);

	return ret;
}

static Node *
lhs(Node *np, Node *new)
{
	switch (np->op) {
	case OMEM:
	case OAUTO:
		*new = *np;
		return new;
	case OPTR:
		return rhs(np->left, new);
	case OFIELD:
		return field(np, new, 1);
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
		code(ASBRANCH, rhs(np, &ret), &ifyes, &ifno);
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
	copy(&ret->type, ret, rhs(colon->left, &aux2));
	code(ASJMP, NULL, &phi, NULL);

	setlabel(ifno.u.sym);
	copy(&ret->type, ret, rhs(colon->right, &aux3));
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

static void
swtch_if(Node *idx)
{
	Node aux1, aux2, *np;
	Symbol *deflabel = NULL;

	for (;;) {
		np = delstmt();
		setlabel(np->label);

		switch (np->op) {
		case OESWITCH:
			if (!deflabel)
				deflabel = np->u.sym;
			aux1.op = OJMP;
			aux1.label = NULL;
			aux1.u.sym = deflabel;
			cgen(&aux1);
			return;
		case OCASE:
			aux1 = *np;
			aux1.op = OBRANCH;
			aux1.label = NULL;
			aux1.left = &aux2;

			aux2.op = OEQ;
			aux2.type = idx->type;
			aux2.left = np->left;
			aux2.right = idx;

			cgen(&aux1);
			break;
		case ODEFAULT:
			deflabel = np->u.sym;
			break;
		default:
			abort();
		}
	}
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
	case OTMP:
	case OCONST:
		*ret = *np;
		return np;
	case OMEM:
	case OAUTO:
		return load(tp, np, ret);
	case ONEG:
	case OAND:
	case OOR:
		true = newlabel();
		false = newlabel();
		phi = label2node(&aux1, NULL);
		tmpnode(ret, &int32type);

		bool(np, true, false);

		setlabel(true);
		code(ASCOPYW, ret, constnode(&aux2, 1, &int32type), NULL);
		code(ASJMP, NULL, phi, NULL);

		setlabel(false);
		code(ASCOPYW, ret, constnode(&aux2, 0, &int32type), NULL);

		setlabel(phi->u.sym);
		return ret;
        case OMOD:
        case OSHR:
		assert(tp->flags & INTF);
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
        case OSHL:
        case OBAND:
        case OBOR:
        case OBXOR:
		assert(tp->flags & INTF);
        case OADD:
        case OSUB:
        case OMUL:
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
                        tbl = (tp->flags & FLOATF) ? opasms : opasmw;
                        break;
                case 8:
                        tbl = (tp->flags & FLOATF) ? opasmd : opasml;
                        break;
                default:
                        abort();
                }
                op = tbl[np->op] + off;
		tmpnode(ret, tp);
                code(op, ret, &aux1, &aux2);
                return ret;
	case OCALL:
	case OCALLE:
		if (l->op == OPTR)
			l = rhs(l, &aux1);
		return call(np, l, ret);
	case OCAST:
		return cast(tp, rhs(l, &aux1), ret);
	case OASSIG:
		/* TODO: Do this transformations in sethi */
		switch (np->u.subop) {
		case OINC:
			op = OADD;
			goto post_oper;
		case ODEC:
			op = OSUB;
		post_oper:
			aux1.op = op;
			aux1.left = rhs(l, ret);
			aux1.right = r;
			aux1.type = np->type;
			rhs(&aux1, &aux2);
			lhs(l, &aux1);
			assign(tp, &aux1, &aux2);
			break;
		default:
			aux2.type = np->type;
			aux2.op = np->u.subop;
			aux2.right = np->right;
			aux2.left = np->left;
			r = rhs(&aux2, &aux1);
			Node aux3;
			if (l->op == OCAST) {
				aux3.type = l->left->type;
				aux3.op = OCAST;
				aux3.left = r;
				aux3.right = NULL;
				r = &aux3;
				l = l->left;
			}
		case 0:
			/* TODO: see what is the most difficult */
			lhs(l, &aux2);
			rhs(r, ret);
			return assign(tp, &aux2, ret);
		}
		return ret;
	case OASK:
		return ternary(np, ret);
	case OCOMMA:
		rhs(l, &aux1);
		return rhs(r, ret);
	case OPTR:
		return load(tp, rhs(l, &aux1), ret);
	case OADDR:
		lhs(l, ret);
		ret->type = *tp;
		return ret;
	case OFIELD:
		return field(np, ret, 0);
	case OBUILTIN:
		switch (np->u.subop) {
		case BVA_START:
			l = rhs(l, &aux1);
			code(ASVSTAR, NULL, l, NULL);
			return NULL;
		case BVA_END:
			return NULL;
		case BVA_ARG:
			l = rhs(l, &aux1);
			code(ASVARG, tmpnode(ret, tp), l, NULL);
			return ret;
		case BVA_COPY:
			/* TODO */
		default:
			abort();
		}
	default:
		abort();
	}
	abort();
}

Node *
cgen(Node *np)
{
	Node aux, *p, *next;

	setlabel(np->label);
	switch (np->op) {
	case OJMP:
		label2node(&aux, np->u.sym);
		code(ASJMP, NULL, &aux, NULL);
		break;
	case OBRANCH:
		next = np->next;
		if (!next->label)
			next->label = newlabel();
		bool(np->left, np->u.sym, next->label);
		break;
	case ORET:
		p = (np->left) ? rhs(np->left, &aux) : NULL;
		code(ASRET, NULL, p, NULL);
		break;
	case OBSWITCH:
		p = rhs(np->left, &aux);
		swtch_if(p);
		break;
	default:
		rhs(np, &aux);
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
		assert(np->type.flags & INTF);
		np->op = OBXOR;
		rp = constnode(NULL, ~(TUINT) 0, &np->type);
		goto binary;
	case OSNEG:
		np->op = OSUB;
		rp = lp;
		lp = constnode(NULL, 0, &np->type);
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
