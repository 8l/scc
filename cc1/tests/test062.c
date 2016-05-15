/*
name: TEST062
description: Test for hexadecimal numbers in upper and lower case
error:
output:
G2	I	F	"main
{
\
	h	#I1
}
*/

int
main(void)
{
	return 0xa == 0xA &&
	       0xb == 0xB &&
	       0xc == 0xC &&
	       0xd == 0xD &&
	       0xe == 0xE &&
	       0xf == 0xF;
}
