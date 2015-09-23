
/*
name: TEST023
description: Basic test for long constants
comments: This test is done for z80 data types
output:
F1
G1	F1	main
{
\
A2	W	i
A3	Z	u
	A2	#W1	:W
	A2	#WFFFFFFFF	:W
	A2	#WFFFFFFFF	:W
	A2	#WFFFFFFFF	:W
	A2	#WFFFFFFFF	:W
	A2	#W3	:W
	A2	#W1	:W
	A2	#W0	:W
	A3	#Z1	:Z
	A3	#ZFFFFFFFF	:Z
	A3	#ZFFFFFFFF	:Z
	A3	#ZFFFFFFFF	:Z
	A3	#ZFFFFFFFF	:Z
	A3	#Z3	:Z
	A3	#Z1	:Z
	A3	#Z0	:Z
	r	#I0
}
*/

int
main(void)
{
	long i;
	unsigned long u;

	i = 1;
	i = -1;
	i = -1l;
	i = -1u;
	i = -1ll;
	i = (1ll << 32) - 1 & 3;
	i = (long) ((1ll << 32) - 1) < 0;
	i = -1u < 0;

	u = 1;
	u = -1;
	u = -1l;
	u = -1u;
	u = -1ll;
	u = (1ll << 32) - 1 & 3;
	u = (long) ((1ll << 32) - 1) < 0;
	u = -1u < 0;
	return 0;
}
