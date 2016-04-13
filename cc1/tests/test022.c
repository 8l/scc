
/*
name: TEST022
description: Basic test for int constants
comments: This test is done for z80 data types
error:
output:
G2	F	"main
{
\
A3	I	"i
A4	N	"u
	A3	#I1	:I
	A3	#IFFFF	:I
	A3	#IFFFF	:I
	A3	#IFFFF	:I
	A3	#IFFFF	:I
	A3	#I3	:I
	A3	#I1	:I
	A3	#I0	:I
	A4	#N1	:N
	A4	#NFFFF	:N
	A4	#NFFFF	:N
	A4	#NFFFF	:N
	A4	#NFFFF	:N
	A4	#N0	:N
	A4	#N3	:N
	A4	#N0	:N
	h	#I0
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
