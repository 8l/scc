/* See LICENSE file for copyright and license details. */

/*
name: TEST001
description: Basic hello world test
error:
output:
X4	I	F	"printf
G6	I	F	"main
{
\
V9	K	#N13
Y8	V9	"	(
	#"hello world
	#K0A
	#K00
)
	X4	Y8	'P	pP	cI
	h	#I0
}
*/

#include <stdio.h>

int
main(void)
{
	printf("hello world\n");
	return 0;
}
