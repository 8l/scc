/*
name: TEST011
description: Basic test for goto
output:
F1
G1	F1	main
{
-
L2
	j	L3
	yI	#I1
L4
	yI	#I0
L3
L5
	j	L4
	yI	#I1
}
*/

#line 1

int
main() {
	start:
		goto next;
		return 1;
	success:
		return 0;
	next:
	foo:
		goto success;
		return 1;
}
