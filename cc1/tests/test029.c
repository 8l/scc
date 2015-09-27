
/*
name: TEST029
description: Test of nested expansion and refusing macro without arguments
comments: f(2) will expand to 2*g, which will expand to 2*f, and in this
          moment f will not be expanded because the macro definition is
          a function alike macro, and in this case there is no arguments.
output:
test029.c:34: error: redefinition of 'f1'
F2
G3	F2	f1
{
\
A4	I	f
	A4	#I2	*I
}
test029.c:35: error: 'f' undeclared
*/


#define f(a) a*g
#define g f

int
f1(void)
{
	int f;

	f(2);
}

int
f1(void)
{
	f(2);
}
