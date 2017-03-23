int f(a,b);

int
f(a,b,c) char b;
{
	return a - c + b;
}

int
main(void)
{
	return f(1,0,1);
}
