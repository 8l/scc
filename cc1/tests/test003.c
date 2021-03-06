/* See LICENSE file for copyright and license details. */

/*
name: TEST003
description: Select function to call inside ternary operator
error:
output:
G2	I	F	"foo
{
\
	h	#I2A
}
G3	I	F	"bar
{
\
	h	#I18
}
G4	I	F	"main
{
\
	h	G2	cI
}
*/

int
foo(void)
{
	return 42;
}

int
bar(void)
{
	return 24;
}

int
main(void)
{
	return (1 ? foo : bar)();
}
