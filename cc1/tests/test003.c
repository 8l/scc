/*
name: TEST003
description: Select function to call inside ternary operator
output:
F1
X1	F1	foo
G1	F1	foo	{
-
	yI	#I2A
}
X2	F1	bar
G2	F1	bar	{
-
	yI	#I18
}
X3	F1	main
G3	F1	main	{
-
	yI	G1	cI
}
*/

int
foo(void)
{
	return 42;
}

int
bar(void)
{
	return 24;
}

int
main(void)
{
	return (1 ? foo : bar)();
}
