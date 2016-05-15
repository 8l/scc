/*
name: TEST015
description: Stress namespace mechanism
error:
test015.c:52: error: label 's' already defined
output:
S8	"s2	#N2	#N1
M9	I	"s	#N0
S5	"s1	#N4	#N1
M6	I	"s	#N0
M10	S8	"s1	#N2
S2	"s	#N4	#N1
M11	S5	"s	#N0
G12	S2	"s2
G14	I	F	"main
{
\
	j	L15
A16	S2	"s
A17	I	"s
	h	A17
	h	A16	M11	.S5	M6	.I	A16	M11	.S5	M10	.S8	M9	.I	+I
L15
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
