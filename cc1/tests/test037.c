/* See LICENSE file for copyright and license details. */

/*
name: TEST037
description: Basic sizeof test
comments: This test is based in z80 sizes
error:
test037.c:30: warning: conditional expression is constant
test037.c:32: warning: conditional expression is constant
test037.c:34: warning: conditional expression is constant
output:
G2	I	F	"main
{
\
	y	L3	#I0
	h	#I1
L3
	y	L4	#I0
	h	#I2
L4
	y	L5	#I0
	h	#I3
L5
	h	#I0
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
