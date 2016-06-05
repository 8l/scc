/* See LICENSE file for copyright and license details. */

/*
name: TEST050
description: Basic test for initializer
error:
TODO:
output:
*/

struct S { int a; int b; };
struct S s = (struct S){1, 2};

int
main()
{
	if(s.a != 1)
		return 1;
	if(s.b != 2)
		return 2;
	return 0;
}
