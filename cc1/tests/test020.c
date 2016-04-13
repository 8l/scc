
/*
name: TEST020
description: Basic test for integer algebraic identities
error:
test020.c:81: warning: division by 0
test020.c:82: warning: division by 0
output:
G2	F	"main
{
\
A3	I	"i
	A3	A3	#I0	!I	:I
	A3	A3	#I0	!I	#I1	,I	:I
	A3	#I1	:I
	A3	A3	#I0	!I	:I
	A3	A3	#I0	!I	#I0	,I	:I
	A3	A3	#I0	!I	:I
	A3	A3	#I0	!I	:I
	A3	#I0	:I
	A3	A3	:I
	A3	#I0	:I
	A3	A3	:I
	A3	#I0	:I
	A3	A3	:I
	A3	A3	:I
	A3	A3	:I
	A3	#I0	A3	-I	:I
	A3	A3	:I
	A3	A3	:I
	A3	A3	:I
	A3	A3	:I
	A3	A3	#I0	,I	:I
	A3	A3	#I0	,I	:I
	A3	A3	:I
	A3	A3	:I
	A3	A3	:I
	A3	#I1	A3	/I	:I
	A3	A3	:I
	A3	A3	:I
	A3	A3	#I1	,I	:I
	A3	A3	#I0	/I	:I
	A3	A3	#I0	%I	:I
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
