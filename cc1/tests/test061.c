/*
name: TEST061
description: Test for macros without arguments but with parenthesis
error:
output:
G3	I	F	"main
{
\
	h	#I1
}
*/

#define X (2)
#define L (0)
#define H (1)
#define Q(x) x

int
main(void)
{
	return X == L + H + Q(1);
}
