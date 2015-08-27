/*
name: TEST018
description: Basic test for arrays
output:
F1
G1	F1	main
{
-
V2	M	#4
V3	V2	#2
A4	V3	arr
A6	P	p
A8	P	q
V9	I	#4
A10	V9	v
	A6	A4	'P	:P
	A8	A4	'P	#P4	+P	#P3	+P	:P
	A4	'P	#P4	+P	#P3	+P	@M	#M2	:M
	A10	'P	@I	#I2	:I
	j	L12	A4	'P	#P4	+P	#P3	+P	@M	MI	#I2	=I
	y	#I1
L12
	j	L13	A6	#P4	+P	#P3	+P	@M	MI	#I2	=I
	y	#I1
L13
	j	L14	A8	@M	MI	#I2	=I
	y	#I1
L14
	j	L15	A10	@I	#I2	=I
	y	#I1
L15
	y	#I0
}
*/

#line 1

int
main()
{
	char arr[2][4], (*p)[4], *q;
	int v[4];

	p = arr;
	q = &arr[1][3];
	arr[1][3] = 2;
	v[0] = 2;

	if (arr[1][3] != 2)
		return 1;
	if (p[1][3] != 2)
		return 1;
	if (*q != 2)
		return 1;
	if (*v != 2)
		return 1;

	return 0;
}
