/*
name: TEST015
description: Stress namespace mechanism
output:
test015.c:52: error: label 's' already defined
S8	s2
M9	I	s
S5	s1
M6	I	s
M10	S8	s1
S2	s
M11	S5	s
G12	S2	s2
F1
G13	F1	main
{
-
	j	L2
A3	S2	s
A4	I	s
	y	A4
	y	A3	M11	.S5	M6	.I	A3	M11	.S5	M10	.S8	M9	.I	+I
L2
*/

typedef struct s s;

struct s {
	struct s1 {
		int s;
		struct s2 {
			int s;
		} s1;
	} s;
} s2;

#define s s

int
main(void)
{
#undef s
	goto s;
	struct s s;
		{
			int s;
			return s;
		}
	return s.s.s + s.s.s1.s;
	s:
		{
			s: return 0;
		}
}
