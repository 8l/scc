/* See LICENSE file for copyright and license details. */

/*
name: TEST066
description: Test cpp defined operator
error:
test066.c:53: error: operator 'defined' requires an identifier
test066.c:53: error: expected ')' before '<EOF>'
output:
G1	I	"x	(
	#I0
)
G3	I	F	"main
{
\
	h	#I0
*/


#if defined X
X
#endif

#if defined(X)
X
#endif

#if X
X
#endif

#define X 0

#if X
X
#endif

#if defined(X)
int x = 0;
#endif

#undef X
#define X 1

#if X
int
main()
{
	return 0;
}
#endif

#if defined (1)
#error 1 is defined
#endif
