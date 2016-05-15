/*
name: TEST051
description: Basic test for initializer
error:
output:
V1	I	#N3
G2	V1	"arr	(
	#I0
	#I1
	#I2
)
G4	I	F	"main
{
\
	y	L5	G2	#I0	=I
	h	#I1
L5
	y	L7	G2	'P	#P2	+P	@I	#I1	=I
	h	#I2
L7
	y	L8	G2	'P	#P4	+P	@I	#I2	=I
	h	#I3
L8
	h	#I0
}
*/

int arr[3] = {[2] = 2, [0] = 0, [1] = 1};

int
main()
{
	if(arr[0] != 0)
		return 1;
	if(arr[1] != 1)
		return 2;
	if(arr[2] != 2)
		return 3;
	return 0;
}
