/* See LICENSE file for copyright and license details. */

/*
name: TEST042
description: Test for bug parsing ternary operators
error:
test042.c:20: error: bad type conversion requested
output:
G2	I	F	"main
{
\
X4	0	F	"f
*/

int
main(void)
{
        void f(void);

        return (int) f();
}
