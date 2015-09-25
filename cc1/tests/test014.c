/*
name: TEST014
description: Basic storage class test
output:
test014.c:16: warning: 'a' defined but not used
test014.c:16: warning: 'k' defined but not used
test014.c:16: warning: 'j' defined but not used
test014.c:16: warning: 'i' defined but not used
test014.c:16: warning: 'h' defined but not used
test014.c:22: warning: 'par' defined but not used
test014.c:22: warning: 'par' defined but not used
test014.c:27: warning: 'par' defined but not used
test014.c:29: error: incorrect storage class for file-scope declaration
G1	I	a
Y2	K	b
X3	I	c
F5
G6	F5	func1
{
\
A7	I	h
T8	K	i
R9	W	j
X10	I	k
T11	Z	a
	r	#I0
}
F13	I
G14	F13	func2
{
R12	I	par
\
A15	I	par
}
T17	F13	func3
{
R16	I	par
\
}
test014.c:29: error: invalid storage class for function 'd'
test014.c:32: error: bad storage class in function parameter
test014.c:33: error: invalid storage class for function 'func4'
test014.c:34: error: invalid type specification
test014.c:35: warning: 'f' defined but not used
test014.c:38: error: conflicting types for 'd'
*/

#line 1
int a;
static char b;
extern int c;
typedef unsigned e;

int
func1(void)
{
	auto h;
	static char i;
	register long j;
	extern int k;
	static unsigned long a;
	return 0;
}

void
func2(register int par)
{
	int par;
}

static void
func3(register int par)
{
}

register short d;

register void
func4(static int par)
{
	static register f;
}

short d;
char d;

