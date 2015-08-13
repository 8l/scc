/*
name: TEST001
description: Basic hello world test
output:
F3	P
X1	F3	printf
F1
X2	F1	main
G2	F1	main	{
-
	X1	"68656C6C6F20776F726C640A	aP	pP	cI
	yI	#I0
}
*/

int printf(char *fmt);

int
main(void)
{
	printf("hello world\n");
	return 0;
}
