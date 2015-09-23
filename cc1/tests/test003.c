/*
name: TEST003
description: Select function to call inside ternary operator
output:
F1
G1	F1	foo
{
-
	r	#I2A
}
G2	F1	bar
{
-
	r	#I18
}
G3	F1	main
{
-
	r	G1	cI
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
