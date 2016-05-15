/*
name: TEST012
description: Basic switch test
error:
test012.c:38: warning: 'foo' defined but not used
output:
G2	I	F	"main
{
\
A3	I	"x
	A3	#I0	:I
	s	A3
	v	L5	#I0
L5
	k	L4
L4
	s	A3
	v	L7	#I0
L7
	s	A3
	v	L9	#I0
L9
	j	L10
	f	L11
L11
	h	#I1
	k	L8
L8
	k	L6
L6
	h	#I2
L10
	s	A3
	v	L13	#I1
L13
	h	#I3
	k	L12
L12
	s	A3
	A3	#I2	:I
L15
	v	L16	#I1
L16
	h	#I4
	k	L14
L14
	s	A3
	v	L18	#I0
L18
	h	A3
	v	L19	#I1
L19
	h	#I1
	f	L20
L20
	h	#I1
	k	L17
L17
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
