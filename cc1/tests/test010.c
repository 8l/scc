/*
name: TEST010
description: Test for continue and break statements
output:
test010.c:9: warning: conditional expression is constant
test010.c:11: warning: conditional expression is constant
test010.c:31: warning: conditional expression is constant
F1
G1	F1	main
{
-
A2	I	x
	A2	#I0	:I
	j	L5
	d
L3
	j	L4
L5
	j	L3	#I1
	b
L4
	j	L8
	d
L6
	j	L9	A2	#I5	!I
	j	L7
L9
	A2	A2	#I1	+I	:I
	j	L6
L8
	j	L6	#I1
	b
L7

	j	L12
	d
L10
	j	L13	A2	#IA	!I
	j	L11
L13
	A2	A2	#I1	+I	:I
	j	L10

L12
	j	L10
	b
L11
	d
L14
	j	L16	A2	#IF	!I
	j	L15
L16
	A2	A2	#I1	+I	:I
	j	L14
	j	L14	#I1
	b
L15
	yI	A2	#IF	-I
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
