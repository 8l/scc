
/*
name: TEST032
description: test special characters @ and $ in macro definitions
output:
F3	I
G4	F3	main
{
\
A6	P	p
	A6	"54686973206973206120737472696E672024206F722023206F72202323616E64206974206973206F6B2021	'P	:P
	r	A6	#P0	!I
}
*/

#define M1(x) "This is a string $ or # or ##" ## #x

int
main(void)
{
	char *p = M1(and it is ok!);

	return p != 0;
}

