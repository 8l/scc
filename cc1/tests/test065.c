/* See LICENSE file for copyright and license details. */

/*
name: TEST065
description: Test decay mixed with * operators
error:
test065.c:65: error: decrement of pointer to an incomplete type
test065.c:65: error: invalid use of undefined type
test065.c:66: warning: 'argv' defined but not used
output:
G7	I	F	"main
{
A1	I	"argc
A5	P	"argv
\
V8	I	#N1
A9	V8	"v
A12	P	"p
A14	P	"f1
A15	P	"f2
	A9	#I0	:I
	A12	A9	'P	:P
	A14	G7	'P	:P
	A15	G7	'P	:P
	y	L18	A1	#I0	!I
	h	#I1
L18
	y	L19	G7	#I0	pI	#P0	pP	cI	#I0	=I
	h	#I2
L19
	y	L20	A14	@F	#I0	pI	#P0	pP	cI	#I0	=I
	h	#I3
L20
	y	L21	A15	@F	#I0	pI	#P0	pP	cI	#I0	=I
	h	#I4
L21
	y	L22	A12	@I	#I0	!I
	h	#I0
L22
*/

int
main(int argc, char *argv[])
{
	int v[1];
	int (*p)[];
	int (*f1)(int ,char *[]);
	int (*f2)(int ,char *[]);

	v[0] = 0;
	p = &v;
	f1 = &main;
	f2 = main;
	if (argc == 0)
		return 1;
	if ((****main)(0, 0))
		return 2;
	if ((****f1)(0, 0))
		return 3;
	if ((****f2)(0, 0))
		return 4;
	if (!(*p)[0])
		return 0;

	return (*++p)[0] || p[1][0];
}
