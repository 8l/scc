
/*
name: TEST037
description: Basic sizeof test
comments: This test is based in z80 sizes
output:
test037.c:29: warning: conditional expression is constant
test037.c:31: warning: conditional expression is constant
test037.c:33: warning: conditional expression is constant
F1	I	E
G2	F1	main
{
\
	j	L3	#I0
	r	#I1
L3
	j	L4	#I0
	r	#I2
L4
	j	L5	#I0
	r	#I3
L5
	r	#I0
}
*/

int main()
{
	if(sizeof(0) != 2)
		return 1;
	if(sizeof(char) != 1)
		return 2;
	if(sizeof(int) != 2)
		return 3;
	return 0;
}
