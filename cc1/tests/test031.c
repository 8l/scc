
/*
name: TEST031
description: Test concatenation in preprocessor
output:
F5	I
G6	F5	main
{
\
A7	I	foo
A8	I	bar
A9	I	foobar
	A9	A7	A8	+I	:I
	A9	A7	A8	+I	:I
	r	#I0
}
*/

#define CAT(x,y) x ## y
#define XCAT(x,y) CAT(x,y)
#define FOO foo
#define BAR bar

int
main(void)
{
	int foo, bar, foobar;

	CAT(foo,bar) = foo + bar;
	XCAT(FOO,BAR) = foo + bar;
	return 0;
}

