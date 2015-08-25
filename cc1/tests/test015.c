/*
name: TEST015
description: Stress namespace mechanism
output:
test015.c:21: warning: 's1' defined but not used
S7	s2
(
M8	I	s
)
S4	s1
(
M5	I	s
M9	S7	s2
)
S2	s
(
M10	S4	s1
)
G11	S2	s
F1
G12	F1	main
{
-
	j	L2
A3	S2	s2
L4
	yI	G11	M10	.S4	M5	.I	G11	M10	.S4	M9	.S7	M8	.I	+I
L2
	yI	A3	M10	.S4	M9	.S7	M8	.I
}
*/

#line 1

struct s {
	struct s1 {
		int s;
		struct s2 {
			int s;
		} s2;
	} s1;
} s;

int
main(void)
{
	goto	s2;
	struct s s2;
	s1:
	return s.s1.s + s.s1.s2.s;
	s2:
	return s2.s1.s2.s;
}
