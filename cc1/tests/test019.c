
/*
name: TEST019
description: Basic test of constant folding in integer arithmetic operations
error:
test019.c:12: warning: division by 0
test019.c:13: warning: division by 0
output:
G2	I	F	"main
{
\
A3	I	"i
	A3	#I3	:I
	A3	#I1	:I
	A3	#I12	:I
	A3	#I2	:I
	A3	#I0	:I
	A3	A3	#I0	%I	:I
	A3	A3	#I0	%I	:I
	A3	#I8	:I
	A3	#I2	:I
	A3	#I4	:I
	A3	#IC	:I
	A3	#I8	:I
	A3	#IFFFD	:I
	A3	#IFFF3	:I
	A3	#I1	:I
	A3	#I0	:I
	A3	#I0	:I
	A3	#I1	:I
	A3	#I0	:I
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
