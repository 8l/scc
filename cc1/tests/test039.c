
/*
name: TEST039
description: Test of integer constants
comments: This test is done for z80 sizes
error:
output:
G2	I	F	"main
{
\
A3	I	"i
A4	N	"u
A5	W	"l
A6	Z	"ul
A7	Q	"ll
A8	O	"ull
	A3	#I1	:I
	A3	#I1	:I
	A4	#N1	:N
	A4	#N1	:N
	A5	#W1	:W
	A5	#W0	:W
	A4	#N0	:N
	A6	#Z1	:Z
	A5	#W1	:W
	A7	#Q0	:Q
	A6	#Z0	:Z
	A8	#O1	:O
	A8	#O1	:O
	h	#I0
}
*/

int
main(void)
{
	int i;
	unsigned u;
	long l;
	unsigned long ul;
	long long ll;
	unsigned long long ull;

	i = 1;
	i = 1u;
	u = 1u;
	u = 1;
	l = 1l;
	l = 0xFFFF + 1;
	u = 0xFFFF + 1;
	ul = 1ul;
	l = 1ul;
	ll = 0xFFFFFFFF + 1;
	ul = 0xFFFFFFFF + 1;
	ull = 1lul;
	ull = 1;
	return 0;
}
