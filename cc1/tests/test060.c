/*
name: TEST060
description: Test for correctness of #line
error:
output:
G2	F	"main
{
\
	h	#I0
}
*/

#undef  line
#define line 1000

#line line
#if 1000 != __LINE__
	#error "  # line line" not work as expected
#endif

int
main()
{
	return 0;
}

