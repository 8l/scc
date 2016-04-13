
/*
name: TEST021
description: Basic test for char constants
comments: This test is done for z80 implementation
error:
output:
G2	F	"main
{
\
A3	K	"uc
A4	C	"sc
	A3	#KFF	:K
	A3	#K23	:K
	A3	#K1	:K
	A3	#K1	:K
	A3	#K41	:K
	A4	#CFF	:C
	A4	#C23	:C
	A4	#C1	:C
	A4	#C1	:C
	A4	#C41	:C
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
