/*
name: TEST047
description: Basic test for initializer
error:
output:
S2	"S	#N6	#N1
M3	I	"a	#N0
M4	I	"b	#N2
M5	I	"c	#N4
G6	S2	"x	(
	#I1
	#I2
	#I3
)
G8	F	"main
{
\
	y	L9	G6	M3	.I	#I1	=I
	h	#I1
L9
	y	L10	G6	M4	.I	#I2	=I
	h	#I2
L10
	y	L11	G6	M5	.I	#I3	=I
	h	#I3
L11
	h	#I0
}
*/



struct S {
	int a;
	int b;
	int c;
};

struct S x = {1, 2, 3};

int
main()
{
	if(x.a != 1) 
		return 1;
	if(x.b != 2) 
		return 2;
	if(x.c != 3) 
		return 3;
	return 0;
}
