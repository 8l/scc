/*
name: TEST001
description: Basic hello world test
output:
F3	P
X4	F3	printf
F5
G6	F5	main
{
\
	X4	"68656C6C6F20776F726C640A	'P	pP	cI
	r	#I0
}
*/

int printf(char *fmt);

int
main(void)
{
	printf("hello world\n");
	return 0;
}
