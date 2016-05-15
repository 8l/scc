/*
name: TEST046
description: Basic test for initializators
error:
output:
V1	I	#N3
G2	V1	"x	(
	#I1
	#I2
	#I3
)
G4	I	F	"main
{
\
	y	L5	G2	#I1	=I
	h	#I1
L5
	y	L7	G2	'P	#P2	+P	@I	#I2	=I
	h	#I2
L7
	y	L8	G2	'P	#P4	+P	@I	#I3	=I
	h	#I3
L8
	h	#I0
}
*/

int x[3] = {1, 2, 3};

int
main()
{
	if(x[0] != 1) 
		return 1;
	if(x[1] != 2) 
		return 2;
	if(x[2] != 3) 
		return 3;
	return 0;
}
