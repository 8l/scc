
/*
name: TEST024
description: Basic test for long long constants
comments: This test is done for z80 data types
output:
F1	I
G2	F1	main
{
\
A3	Q	i
A4	O	u
	A3	#Q1	:Q
	A3	#QFFFFFFFF	:Q
	A3	#QFFFFFFFF	:Q
	A3	#QFFFFFFFF	:Q
	A3	#QFFFFFFFF	:Q
	A3	#Q3	:Q
	A3	#Q1	:Q
	A4	#O1	:O
	A4	#OFFFFFFFF	:O
	A4	#OFFFFFFFF	:O
	A4	#OFFFFFFFF	:O
	A4	#OFFFFFFFF	:O
	A4	#O3	:O
	A4	#O0	:O
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
