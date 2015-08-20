/*
name: TEST014
description: Basic storage class test
output:
test014.c:22: warning: 'a' defined but not used
test014.c:22: warning: 'k' defined but not used
test014.c:22: warning: 'j' defined but not used
test014.c:22: warning: 'i' defined but not used
test014.c:22: warning: 'h' defined but not used
test014.c:28: warning: 'par' defined but not used
test014.c:28: warning: 'par' defined but not used
test014.c:33: warning: 'par' defined but not used
test014.c:35: error: incorrect storage class for file-scope declaration
test014.c:35: error: invalid storage class for function 'd'
test014.c:38: error: bad storage class in function parameter
test014.c:39: error: invalid storage class for function 'func4'
test014.c:40: error: invalid type specification
test014.c:41: warning: 'f' defined but not used
test014.c:44: error: conflicting types for 'd'
G1	I	a
Y2	M	b
X3	I	c
F1
G5	F1	func1
{
-
A2	I	h
T3	M	i
R4	W	j
X5	I	k
T6	Z	a
	yI	#I0
}
F2	I
G6	F2	func2
{
R1	I	par
-
A3	I	par
}
T7	F2	func3
{
R1	I	par
-
}
????
*/

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

