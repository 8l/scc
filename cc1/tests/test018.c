/*
name: TEST018
description: Basic test for arrays
output:
F1	I	E
G2	F1	main
{
\
V3	K	#4
V4	V3	#2
A5	V4	arr
A7	P	p
A9	P	q
V10	I	#4
A11	V10	v
	A7	A5	'P	:P
	A9	A5	'P	#P4	+P	#P3	+P	:P
	A5	'P	#P4	+P	#P3	+P	@K	#K2	:K
	A11	#I2	:I
	j	L13	A5	'P	#P4	+P	#P3	+P	@K	gI	#I2	=I
	r	#I1
L13
	j	L14	A7	#P4	+P	#P3	+P	@K	gI	#I2	=I
	r	#I1
L14
	j	L15	A9	@K	gI	#I2	=I
	r	#I1
L15
	j	L16	A11	#I2	=I
	r	#I1
L16
	r	#I0
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
