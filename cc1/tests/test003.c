/*
name: TEST003
description: Select function to call inside ternary operator
output:
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
