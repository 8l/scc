/*
name: TEST010
description: Test for continue and break statements
output:
test010.c:9: warning: conditional expression is constant
test010.c:11: warning: conditional expression is constant
test010.c:31: warning: conditional expression is constant
F1
G2	F1	main
{
\
A3	I	x
	A3	#I0	:I
	j	L6
	e
L4
	j	L5
L6
	j	L4	#I1
	b
L5
	j	L9
	e
L7
	j	L10	A3	#I5	!I
	j	L8
L10
	A3	A3	#I1	+I	:I
	j	L7
L9
	j	L7	#I1
	b
L8

	j	L13
	e
L11
	j	L14	A3	#IA	!I
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
	j	L17	A3	#IF	!I
	j	L16
L17
	A3	A3	#I1	+I	:I
	j	L15
	j	L15	#I1
	b
L16
	r	A3	#IF	-I
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
