
/*
name: TEST038
description: Basic test for tentative definitions
error:
test038.c:43: error: redeclaration of 'x'
output:
G1	I	"x
G1	I	"x	(
	#I0
)
G5	F	"foo
{
\
	h	X3	'P
}
G3	F	"main
{
\
	G1	#I0	:I
	h	G1
}
*/

int x;
int x = 0;
int x;

int main();

void *
foo()
{
	return &main;
}

int
main()
{
	x = 0;
	return x;
}
int x = 1;
