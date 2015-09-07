
/*
name: TEST019
description: Basic test of constant folding in integer arithmetic operations
output:
test019.c:13: warning: division by 0
test019.c:14: warning: division by 0
F1
G1	F1	main
{
-
A2	I	i
	A2	#I3	:I
	A2	#I1	:I
	A2	#I12	:I
	A2	#I2	:I
	A2	#I0	:I
	A2	A2	#I0	%I	:I
	A2	A2	#I0	%I	:I
	A2	#I8	:I
	A2	#I2	:I
	A2	#I4	:I
	A2	#IC	:I
	A2	#I8	:I
	A2	#IFFFD	:I
	A2	#IFFF3	:I
	A2	#I1	:I
	A2	#I0	:I
	A2	#I0	:I
	A2	#I1	:I
	A2	#I0	:I
}
*/

#line 1

int
main(void)
{
	int i;

	i = 1 + 2;
	i = 2 - 1;
	i = 3 * 6;
	i = 10 / 5;
	i = 10 % 5;
	i = i % 0;
	i = i % 0;
	i = 1 << 3;
	i = 8 >> 2;
	i = 12 & 4;
	i = 8 | 4;
	i = 12 ^ 4;
	i = -(3);
	i = ~12;
	i = 1 < 3;
	i = 2 > 3;
	i = 2 >= 3;
	i = 2 <= 3;
	i = 1 == 0;
}
