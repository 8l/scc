/*
name: TEST016
description: Basic pointer test
output:
test016.c:43: error: redefinition of 'func2'
G1	I	g
F2
G3	F2	func1
{
\
A4	I	x
A6	P	p
	G1	#I1	:I
	A4	#I1	:I
	A6	A4	'P	:P
	A6	@I	#I0	:I
	j	L7	A4	#I0	=I
	r	#I1
L7
	A6	G1	'P	:P
	A6	@I	#I0	:I
	j	L8	A6	#P0	!I
	r	#I1
L8
	r	#I0
}
G9	F2	func2
{
\
A10	I	x
A11	P	p
A13	P	pp
	A10	#I1	:I
	A11	A10	'P	:P
	A13	A11	'P	:P
	j	L14	A11	#P0	=I
	A13	@P	@I	#I0	:I
L14
	A11	#P0	:P
	r	A10
}
test016.c:47: error: incompatible types when assigning
*/

#line 1

int g;

int
func1(void)
{
	int  x;
	int *p;

	g = 1;
	x = 1;
	p = &x;
	*p = 0;
	if (x)
		return 1;
	
	p = &g;
	*p = 0;
	if (p == 0)
		return 1;
	return 0;
}

int
func2(void)
{
	int   x;
	int  *p;
	int **pp;
	
	x = 1;
	p = &x;
	pp = &p;
	if (p != 0)
		**pp = 0;
	p = 0;
	return x;
}

int
func2(void)
{
	char c;
	int *p;

	p = &c;
	return *p;
}
