/*
PATTERN:
0014-macro.c:9: error: macro "X" received 1 arguments, but it takes 0
.
*/

#define X() 0

X(A)

