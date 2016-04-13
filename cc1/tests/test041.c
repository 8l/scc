/*
name: TEST041
description: Test for bug parsing ternary operators
error:
test041.c:48: error: type mismatch in conditional expression
test041.c:48: error: incompatible types when assigning
test041.c:49: error: used struct/union type value where scalar is required
test041.c:50: warning: 'i' defined but not used
test041.c:50: warning: 'foo' defined but not used
test041.c:50: warning: 's' defined but not used
output:
G2	F	"main
{
\
A3	I	"i
A5	P	"q
A7	P	"s
A8	P	"p
S10	"foo	#N2	#N1
M11	I	"i	#N0
A12	S10	"foo
	A3	A3	#I0	!I	#W0	#W0	?W	gI	:I
	A8	A3	#I0	!I	#P0	#P0	?P	:P
	A8	A3	#I0	!I	#P0	#P0	?P	:P
	A8	A3	#I0	!I	#P0	#P0	?P	:P
	A5	A3	#I0	!I	#P0	A8	?P	:P
	A5	A3	#I0	!I	A8	#P0	?P	:P
	A5	A3	#I0	!I	A5	#P0	?P	:P
	A5	A3	#I0	!I	#P0	A5	?P	:P
*/
   
int
main(void)
{
	int i, *q;
	char *s;
	void *p;
	struct foo {int i;} foo;

	i = i ? 0 : 0l;
	p = i ? (void *) 0 : 0;
	p = i ? 0 : (void *) 0;
	p = i ? 0 : (const void *) 0;
	q = i ? 0 : p;
	q = i ? p : 0;
	q = i ? q : 0;
	q = i ? 0 : q;
	p = i ? 2 : p;
	foo ? 1 : 2;
}

