/*
name: TEST012
description: Basic switch test
output:
test012.c:39: warning: 'foo' defined but not used
F1
G1	F1	main
{
\
A2	I	x
	A2	#I0	:I
	s	L4	A2
L5
	j	L3
L4
	t	#1
	v	L5	#I0
L3
	s	L7	A2
L8
	s	L10	A2
L11
	j	L12
L13
	r	#I1
	j	L9
L10
	t	#2
	v	L11	#I0
	f	L13
L9
	j	L6
L7
	t	#1
	v	L8	#I0
L6
	r	#I2
L12
	s	L15	A2
L16
	r	#I3
	j	L14
L15
	t	#1
	v	L16	#I1
L14
	s	L18	A2
	A2	#I2	:I
L19
L20
	r	#I4
	j	L17
L18
	t	#1
	v	L20	#I1
L17
	s	L22	A2
L23
	r	A2
L24
	r	#I1
L25
	r	#I1
	j	L21
L22
	t	#3
	v	L24	#I1
	v	L23	#I0
	f	L25
L21
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
