/* See LICENSE file for copyright and license details. */
#include <assert.h>
#include <stdlib.h>

#include "arch.h"
#include "../../cc2.h"
#include "../../../inc/sizes.h"

enum lflags {
	FORCE = 1 << 0,
	LOADL = 1 << 1,
	LOADR = 1 << 2
};

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

static Node *
tmpnode(Node *np)
{
	Symbol *sym;

	sym = getsym(TMPSYM);
	sym->type = np->type;
	sym->kind = STMP;
	np->u.sym = sym;
	np->op = OTMP;
	np->flags |= ISTMP;
	return np;
}

/*
 * load() load the address passed in a child of np in a temporary
 * if it is not already in a temporay. It can be forced to load
 * using the FORCE flag
 */
static Node *
load(Node *np, int flags)
{
	int op;
	Type *tp;
	Node *child;

	child = (flags & LOADL) ? np->left : np->right;
	if (!child)
		return NULL;
	tp = &child->type;

	if ((flags & FORCE) || !(child->flags & (ISTMP|ISCONS))) {
		Node *new = tmpnode(newnode(OTMP));
		new->type = *tp;
		new->left = child;

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
		code(op, new, child, NULL);
		child = new;
	}

	return (flags & LOADL) ? (np->left = child) : (np->right = child);
}

static Node *
cast(Node *nd)
{
	Type *ts, *td;
	Node *tmp, *ns;
	int op, disint, sisint;
	extern Type uint32type, int32type;

	ns = load(nd, LOADL);
	td = &nd->type;
	ts = &ns->type;
	disint = (td->flags & INTF) != 0;
	sisint = (ts->flags & INTF) != 0;

	if (disint && sisint) {
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
	} else if (disint) {
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
	} else if (sisint) {
		/* conversion from int to float */
		switch (ts->size) {
		case 1:
		case 2:
			tmp = tmpnode(newnode(ONOP));
			tmp->type = (ts->flags&SIGNF) ? int32type : uint32type;
			tmp->left = ns;
			nd->left = ns = cast(tmp);
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
	code(op, tmpnode(nd), ns, NULL);
	return nd;
}

static Node *
call(Node *np)
{
	int n, op;
	Type *tp = &np->type;
	Node **q, *tmp, *p, *pars[NR_FUNPARAM];

	for (n = 0, p = np->right; p; p = p->right)
		pars[n++] = cgen(p->left);

	switch (tp->size) {
	case 0:
		np->left = tmpnode(newnode(OTMP));
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
	code(op, tmpnode(np), np->left, NULL);

	for (q = pars; q < &pars[n]; ++q) {
		op = (q == &pars[n-1]) ? ASPARE : ASPAR;
		p = newnode(OTMP);
		p->type = (*q)->type;
		code(op, NULL, *q, tmpnode(p));
	}
	code(ASCALL, NULL, NULL, NULL);

	return np;
}

static Node *
abbrev(Node *np)
{
	Node *tmp;

	if (np->u.subop == 0)
		return np->right;
	tmp = newnode(np->u.subop);
	tmp->type = np->type;
	tmp->right = np->right;
	tmp->left = np->left;
	return np->right = cgen(tmp);
}

/* TODO: Fix "memory leaks" */
Node *
cgen(Node *np)
{
	Node *ifyes, *ifno, *next;
	Symbol *sym;
	Type *tp;
	int op, off;
	char *tbl;

	if (!np)
		return NULL;

	setlabel(np->label);
	 if (np->op != OCALL) {
		np->left = cgen(np->left);
		np->right = cgen(np->right);
	}
	tp = &np->type;

	switch (np->op) {
	case OSTRING:
		abort();
	case OCONST:
	case OLABEL:
		np->flags |= ISCONS;
	case OMEM:
	case OAUTO:
		return np;
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
		code(op, tmpnode(np), load(np, LOADL), load(np, LOADR));
		return np;
	case ONOP:
	case OBLOOP:
	case OELOOP:
		return NULL;
	case OCAST:
		return cast(np);
	case OADDR:
		np->flags |= ISTMP;
		np->op = OTMP;
		np->u.sym = np->left->u.sym;
		return np;
	case OPTR:
		load(np, LOADL);
		/* FIXME: The type of the loaded value is not np->type */
		load(np, LOADL|FORCE);
		return tmpnode(np);
	case OCPL:
	case OPAR:
	case ONEG:
	case OINC:
	case ODEC:
		abort();
	case OASSIG:
		abbrev(np);
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
		code(op, np->left, load(np, LOADR), NULL);
		return np->right;
	case OCOMMA:
		return np->right;
	case OCALL:
		return call(np);
	case OFIELD:
	case OASK:
	case OCOLON:
	case OAND:
	case OOR:
		abort();
	case OBRANCH:
		next = np->next;
		load(np, LOADL);
		if (next->label) {
			sym = getsym(TMPSYM);
			sym->kind = SLABEL;
			next->label = sym;
		}
		ifyes = label2node(np->u.sym);
		ifno = label2node(next->label);
		op = ASBRANCH;
		np = np->left;
		goto emit_jump;
	case OJMP:
		ifyes = label2node(np->u.sym);
		op = ASJMP;
		np = ifno = NULL;
	emit_jump:
		code(op, np, ifyes, ifno);
		deltree(ifyes);
		deltree(ifno);
		return NULL;
	case ORET:
		code(ASRET, NULL, load(np, LOADL), NULL);
		return NULL;
	case OCASE:
	case ODEFAULT:
	case OESWITCH:
	case OBSWITCH:
	default:
		abort();
	}
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
 * QBE is AMD64 targered we could do a better job here, and try to
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
	default:
		sethi(lp);
		sethi(rp);
		break;
	}

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
