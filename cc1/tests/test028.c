/* See LICENSE file for copyright and license details. */

/*
name: TEST028
description: Test of reinterpretation in define
error:
output:
G6	P	F	"foo
{
\
V7	K	#N3
Y10	V7	"	(
	#"hi
	#K00
)
	h	Y10	'P
}
*/


#define M(x) x
#define A(a,b) a(b)

char *
foo(void)
{
	return A(M,"hi");
}
