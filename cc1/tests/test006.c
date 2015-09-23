/*
name: TEST006
description: Basic test for if
output:
test006.c:6: warning: conditional expression is constant
test006.c:8: warning: conditional expression is constant
test006.c:11: warning: conditional expression is constant
G1	K	c
F1
G2	F1	main
{
\
	j	L2	#I0
	r	#I1
	j	L3
L2
	j	L4	#I0
	j	L5
L4
	j	L6	#I1
	j	L7	G1	KI	#I0	=I
	r	#I1
	j	L8
L7
	r	#I0
L8
	j	L9
L6
	r	#I1
L9
L5
L3
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
