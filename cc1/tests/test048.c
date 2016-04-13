/*
name: TEST048
description: Basic test for initializer
error:
output:
S2	"S	#N4	#N1
M3	I	"a	#N0
M4	I	"b	#N2
V5	S2	#N1
G6	V5	"x	(
	#I1
	#I2
)
G8	F	"main
{
\
	y	L9	G6	M3	.I	#I1	=I
	h	#I1
L9
	y	L11	G6	M4	.I	#I2	=I
	h	#I2
L11
	h	#I0
}
*/

struct S {
	int a;
	int b;
};

struct S x[1] = {{1, 2}};

int
main()
{
	if(x[0].a != 1) 
		return 1;
	if(x[0].b != 2) 
		return 2;
	return 0;
}
