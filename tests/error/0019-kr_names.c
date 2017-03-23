/*
PATTERN:
0019-kr_names.c:9: warning: parameter names (without types) in function declaration
0019-kr_names.c:13: warning: type of 'a' defaults to int
0019-kr_names.c:13: warning: type of 'c' defaults to int
.
*/

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
