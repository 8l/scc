
/*
name: TEST024
description: Basic test for long long constants
comments: This test is done for z80 data types
output:
F1
G1	F1	main
{
-
A2	Q	i
A3	O	u
	A2	#Q1	:Q
	A2	#QFFFFFFFF	:Q
	A2	#QFFFFFFFF	:Q
	A2	#QFFFFFFFF	:Q
	A2	#QFFFFFFFF	:Q
	A2	#Q3	:Q
	A2	#Q1	:Q
	A3	#O1	:O
	A3	#OFFFFFFFF	:O
	A3	#OFFFFFFFF	:O
	A3	#OFFFFFFFF	:O
	A3	#OFFFFFFFF	:O
	A3	#O3	:O
	A3	#O0	:O
	r	#I0
}
*/

int
main(void)
{
	long long i;
	unsigned long long u;

	i = 1;
	i = -1;
	i = -1l;
	i = -1u;
	i = -1ll;
	i = -1ll & 3;
	i = -1ll < 0;

	u = 1;
	u = -1;
	u = -1l;
	u = -1u;
	u = -1ll;
	u = -1llu & 3;
	u = -1llu < 0;
	return 0;
}
