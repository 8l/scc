
/*
name: TEST022
description: Basic test for int constants
comments: This test is done for z80 data types
output:
F1
G1	F1	main
{
-
A2	I	i
A3	N	u
	A2	#I1	:I
	A2	#IFFFF	:I
	A2	#IFFFF	:I
	A2	#IFFFF	:I
	A2	#IFFFF	:I
	A2	#I3	:I
	A2	#I1	:I
	A2	#I0	:I
	A3	#N1	:N
	A3	#NFFFF	:N
	A3	#NFFFF	:N
	A3	#NFFFF	:N
	A3	#NFFFF	:N
	A3	#N0	:N
	A3	#N3	:N
	A3	#N0	:N
	r	#I0
}
*/

int
main(void)
{
	int i;
	unsigned u;

	i = 1;
	i = -1;
	i = -1l;
	i = -1u;
	i = -1ll;
	i = 32766 + 1 & 3;
	i = (int) 32768 < 0;
	i = -1u < 0;

	u = 1;
	u = -1;
	u = -1l;
	u = -1u;
	u = -1ll;
	u = (unsigned) 32768 < 0;
	u = 32766 + 1 & 3;
	u = -1u < 0;
	return 0;
}
