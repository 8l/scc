/* See LICENSE file for copyright and license details. */

/*
name: TEST030
description: Basic test for vararg functions
error:
output:
S2	"foo	#N18	#N2
M3	I	"i	#N0
M4	I	"j	#N2
M5	I	"k	#N4
M7	P	"p	#N6
M8	J	"v	#N8
G14	I	F	"f1
{
A9	S2	"f
A11	P	"p
A12	I	"n
\
	y	L15	A9	M3	.I	A11	@S2	M3	.I	=I
	h	#I0
L15
	h	A11	@S2	M4	.I	A12	+I
}
G17	I	F	"main
{
\
A18	S2	"f
	A18	M3	.I	A18	M4	.I	#I1	:I	:I
	G14	A18	pS2	A18	'P	pP	#I2	pI	cI
	G14	A18	pS2	A18	'P	pP	#I2	pI	#I1	pI	A18	pS2	A18	'P	pP	cI
	h	#I0
}
*/

struct foo {
	int i, j, k;
	char *p;
	float v;
};

int
f1(struct foo f, struct foo *p, int n, ...)
{
	if (f.i != p->i)
		return 0;
	return p->j + n;
}

int
main(void)
{
	struct foo f;

	f.i = f.j = 1;
	f1(f, &f, 2);
	f1(f, &f, 2, 1, f, &f);

	return 0;
}
