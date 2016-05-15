
/*
name: TEST035
description: Basic test for enumerations
error:
output:
G7	I	F	"main
{
\
A8	I	"e
	A8	#I3	:I
	y	L9	A8	#I1	=I
	h	#I0
L9
	y	L10	A8	#I0	=I
	h	#I1
L10
	y	L11	A8	#I2	=I
	h	#I2
L11
	y	L12	A8	#I3	=I
	h	#I3
L12
	h	#I0
}
*/

enum E {
	x,
	y = 2,
	z,
};


int
main()
{
	enum E e = 3;

	if (e !=1)
		return 0;
	if (e != x)
		return 1;
	if (e != y)
		return 2;
	if (e != z)
		return 3;
	
	return x;
}
