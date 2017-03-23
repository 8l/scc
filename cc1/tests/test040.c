/* See LICENSE file for copyright and license details. */

/*
name: TEST040
description: Test for bug parsing typenames in struct definition
error:
output:
S2	"List	#NC	#N2
M4	I	"len	#N0
M6	P	"head	#N2
M7	P	"back	#N4
G9	I	F	"main
{
\
A10	S2	"List
	h	A10	M4	.I
}
*/

typedef struct List List;
struct List {
	int len;
	struct List *head;
	List *back;
};

int
main(void)
{
	List List;

	return List.len;
}

