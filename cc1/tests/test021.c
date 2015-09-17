
/*
name: TEST021
description: Basic test for char constants
comments: This test is done for z80 implementation
output:
F1
G1	F1	main
{
-
A2	K	uc
A3	C	sc
	A2	#KFF	:K
	A2	#K23	:K
	A2	#K1	:K
	A2	#K1	:K
	A2	#K41	:K
	A3	#CFF	:C
	A3	#C23	:C
	A3	#C1	:C
	A3	#C1	:C
	A3	#C41	:C
}
*/

int
main(void)
{
	unsigned char uc;
	signed char sc;

	uc = -1;
	uc = '\x23';
	uc = 1u;
	uc = 1025;
	uc = 'A';

	sc = -1;
	sc = '\x23';
	sc = 1u;
	sc = 1025;
	sc = 'A';
}
