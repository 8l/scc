
/*
name: TEST030
description: Basic test for vararg functions
output:
F13	I	S2	P	I	E
G14	F13	f1
{
S2	foo
M3	I	i
M4	I	j
M5	I	k
M7	P	p
M8	J	v
A9	S2	f
A11	P	p
A12	I	n
\
	j	L15	A9	M3	.I	A11	@S2	M3	.I	=I
	r	#I0
L15
	r	A11	@S2	M4	.I	A12	+I
}
F16	I
G17	F16	main
{
\
A18	S2	f
	A18	M3	.I	A18	M4	.I	#I1	:I	:I
	G14	A18	pS2	A18	'P	pP	#I2	pI	cI
	G14	A18	pS2	A18	'P	pP	#I2	pI	#I1	pI	A18	pS2	A18	'P	pP	cI
	r	#I0
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
