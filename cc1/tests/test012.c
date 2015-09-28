/*
name: TEST012
description: Basic switch test
output:
test012.c:39: warning: 'foo' defined but not used
F1	I	E
G2	F1	main
{
\
A3	I	x
	A3	#I0	:I
	s	L5	A3
L6
	j	L4
L5
	t	#1
	v	L6	#I0
L4
	s	L8	A3
L9
	s	L11	A3
L12
	j	L13
L14
	r	#I1
	j	L10
L11
	t	#2
	v	L12	#I0
	f	L14
L10
	j	L7
L8
	t	#1
	v	L9	#I0
L7
	r	#I2
L13
	s	L16	A3
L17
	r	#I3
	j	L15
L16
	t	#1
	v	L17	#I1
L15
	s	L19	A3
	A3	#I2	:I
L20
L21
	r	#I4
	j	L18
L19
	t	#1
	v	L21	#I1
L18
	s	L23	A3
L24
	r	A3
L25
	r	#I1
L26
	r	#I1
	j	L22
L23
	t	#3
	v	L25	#I1
	v	L24	#I0
	f	L26
L22
}
*/

#line 1

int
main()
{
	int x;

	x = 0;
	switch(x)
	case 0:
		;
	switch(x)
	case 0:
		switch(x) {
		case 0:
			goto next;
		default:
			return 1;
		}
	return 2;
	next:
	switch(x)
	case 1:
		return 3;
	switch(x) {
		x = 1 + 1;
		foo:
	case 1:
		return 4;
	}
	switch(x) {
	case 0:
		return x;
	case 1:
		return 1;
	default:
		return 1;
	}
}
