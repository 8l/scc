/*
name: TEST006
description: Basic test for if
output:
test006.c:6: warning: conditional expression is constant
test006.c:8: warning: conditional expression is constant
test006.c:11: warning: conditional expression is constant
G1	K	c
F2	E
G3	F2	main
{
\
	j	L4	#I0
	r	#I1
	j	L5
L4
	j	L6	#I0
	j	L7
L6
	j	L8	#I1
	j	L9	G1	gK	#I0	=I
	r	#I1
	j	L10
L9
	r	#I0
L10
	j	L11
L8
	r	#I1
L11
L7
L5
	r	#I1
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
