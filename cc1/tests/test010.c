/* See LICENSE file for copyright and license details. */

/*
name: TEST010
description: Test for continue and break statements
error:
test010.c:8: warning: conditional expression is constant
test010.c:10: warning: conditional expression is constant
test010.c:30: warning: conditional expression is constant
output:
G2	I	F	"main
{
\
A3	I	"x
	A3	#I0	:I
	j	L6
	e
L4
	j	L5
L6
	y	L4	#I1
	b
L5
	j	L9
	e
L7
	y	L10	A3	#I5	!I
	j	L8
L10
	A3	A3	#I1	+I	:I
	j	L7
L9
	y	L7	#I1
	b
L8

	j	L13
	e
L11
	y	L14	A3	#IA	!I
	j	L12
L14
	A3	A3	#I1	+I	:I
	j	L11

L13
	j	L11

	b
L12
	e
L15
	y	L17	A3	#IF	!I
	j	L16
L17
	A3	A3	#I1	+I	:I
	j	L15
	y	L15	#I1
	b
L16
	h	A3	#IF	-I
}
*/

#line 1

int
main()
{
	int x;

	x = 0;
	while(1)
		break;
	while(1) {
		if (x == 5) {
			break;
		}
		x = x + 1;
		continue;
	}
	for (;;) {
		if (x == 10) {
			break;
		}
		x = x + 1;
		continue;
	}
	do {
		if (x == 15) {
			break;
		}
		x = x + 1;
		continue;
	} while(1);
	return x - 15;
}
