/*
name: TEST008
description: Basic do while loop
error:
output:
G2	I	F	"main
{
\
A3	I	"x
	A3	#I0	:I
	e
L4
	A3	A3	#I1	+I	:I
	y	L4	A3	#IA	<I
	b
L5
	e
L6
	A3	A3	#I1	+I	:I
	y	L6	A3	#I14	<I
	b
L7
	h	A3	#I14	-I
}
*/

int 
main()
{
	int x;
	
	x = 0;
	do 
	  x = x + 1;
	while(x < 10);
	
	do {
	  x = x + 1;
	} while(x < 20);
	
	return x - 20;
}

