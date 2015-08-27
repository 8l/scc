/*
name: TEST017
description: Basic test about pointers and structs
output:
F1
G9	F1	main
{
-
S2	s1
(
M3	I	y
M4	I	z
)
A2	S2	nested
S6	s2
(
M8	P	p
)
A3	S6	v
	A3	M8	.P	A2	'P	:P
	A3	M8	.P	@S2	M3	.I	#I1	:I
	A3	M8	.P	@S2	M4	.I	#I2	:I
	j	L4	A2	M3	.I	#I1	=I
	y	#I1
L4
	j	L5	A2	M4	.I	#I2	=I
	y	#I2
L5
	y	#I0
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
