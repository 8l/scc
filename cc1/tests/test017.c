/*
name: TEST017
description: Basic test about pointers and structs
error:
output:
G10	I	F	"main
{
\
S2	"s1	#N4	#N1
M3	I	"y	#N0
M4	I	"z	#N2
A11	S2	"nested
S6	"s2	#N4	#N2
M8	P	"p	#N0
A12	S6	"v
	A12	M8	.P	A11	'P	:P
	A12	M8	.P	@S2	M3	.I	#I1	:I
	A12	M8	.P	@S2	M4	.I	#I2	:I
	y	L13	A11	M3	.I	#I1	=I
	h	#I1
L13
	y	L14	A11	M4	.I	#I2	=I
	h	#I2
L14
	h	#I0
}
*/

#line 1

struct s1 {
    int y;
    int z;
};

struct s2 {
    struct s1 *p;
};

int main()
{
    struct s1 nested;
    struct s2 v;
    v.p = &nested;
    v.p->y = 1;
    v.p->z = 2;
    if (nested.y != 1)
        return 1;
    if (nested.z != 2)
        return 2;
    return 0;
}
