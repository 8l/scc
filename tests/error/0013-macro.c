/*
PATTERN:
0013-macro.c:9: error: macro "X" received 0 arguments, but it takes 1
.
*/

#define X(A, ...) 0

X()

