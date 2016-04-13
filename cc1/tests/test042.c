/*
name: TEST042
description: Test for bug parsing ternary operators
error:
test042.c:17: error: bad type convertion requested
output:
G2	F	"main
{
\
*/

int
main(void)
{
        void f(void);

        return (int) f();
}
