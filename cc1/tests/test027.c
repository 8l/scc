/* See LICENSE file for copyright and license details. */

/*
name: TEST027
description: Test of cpp stringizer
error:
output:
G3	I	F	"main
{
\
A5	P	"p
V6	K	#N19
Y7	V6	"	(
	#"hello is better than bye
	#K00
)
	A5	Y7	'P	:P
	h	A5	@K	gI
}
*/

#define x(y) #y

int
main(void)
{
	char *p;
	p = x(hello)  " is better than bye";

	return *p;
}
