/* See LICENSE file for copyright and license details. */

/*
name: TEST006
description: Basic test for if
error:
test006.c:5: warning: conditional expression is constant
test006.c:7: warning: conditional expression is constant
test006.c:10: warning: conditional expression is constant
output:
G1	K	"c
G3	I	F	"main
{
\
	y	L4	#I0
	h	#I1
	j	L5
L4
	y	L6	#I0
	j	L7
L6
	y	L8	#I1
	y	L9	G1	gI	#I0	=I
	h	#I1
	j	L10
L9
	h	#I0
L10
	j	L11
L8
	h	#I1
L11
L7
L5
	h	#I1
}
*/

char c;

#line 1

int
main()
{
	if(0) {
		return 1;
	} else if(0) {
		/* empty */
	} else {
		if(1) {
			if(c)
				return 1;
			else
				return 0;
		} else {
			return 1;
		}
	}
	return 1;
}
