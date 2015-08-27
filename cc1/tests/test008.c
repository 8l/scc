/*
name: TEST008
description: Basic do while loop
output:
F1
G1	F1	main
{
-
A2	I	x
	A2	#I0	:I
	d
L3
	A2	A2	#I1	+I	:I
	j	L3	A2	#IA	<I
	b
L4
	d
L5
	A2	A2	#I1	+I	:I
	j	L5	A2	#I14	<I
	b
L6
	y	A2	#I14	-I
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

