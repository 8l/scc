
/*
name: TEST020
description: Basic test for integer algebraic identities
output:
test020.c:81: warning: division by 0
test020.c:82: warning: division by 0
F1
G1	F1	main
{
\
A2	I	i
	A2	A2	#I0	!I	:I
	A2	A2	#I0	!I	#I1	,I	:I
	A2	#I1	:I
	A2	A2	#I0	!I	:I
	A2	A2	#I0	!I	#I0	,I	:I
	A2	A2	#I0	!I	:I
	A2	A2	#I0	!I	:I
	A2	#I0	:I
	A2	A2	:I
	A2	#I0	:I
	A2	A2	:I
	A2	#I0	:I
	A2	A2	:I
	A2	A2	:I
	A2	A2	:I
	A2	#I0	A2	-I	:I
	A2	A2	:I
	A2	A2	:I
	A2	A2	:I
	A2	A2	:I
	A2	A2	#I0	,I	:I
	A2	A2	#I0	,I	:I
	A2	A2	:I
	A2	A2	:I
	A2	A2	:I
	A2	#I1	A2	/I	:I
	A2	A2	:I
	A2	A2	:I
	A2	A2	#I1	,I	:I
	A2	A2	#I0	/I	:I
	A2	A2	#I0	%I	:I
}
*/

int
main(void)
{
	int i;

	i = i || 0;
	i = i || 4;
	i = 4 || i;
	i = 0 || i;
	i = i && 0;
	i = i && 4;
	i = 4 && i;
	i = 0 && i;
	i = i << 0;
	i = 0 << i;
	i = i >> 0;
	i = 0 >> i;	
	i = i + 0;
	i = 0 + i;
	i = i - 0;
	i = 0 - i;
	i = i | 0;
	i = 0 | i;
	i = i ^ 0;
	i = 0 ^ i;
	i = i * 0;
	i = 0 * i;
	i = i * 1;
	i = 1 * i;
	i = i / 1;
	i = 1 / i;
	i = i & ~0;
	i = ~0 & i;
	i = i % 1;
	i = i / 0;
	i = i % 0;
}
