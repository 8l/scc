/* See LICENSE file for copyright and license details. */

/*
name: TEST033
description: test for #if defined()
error:
output:
G1	I	"c
*/

#if defined(FOO)
int a;
#elif !defined(FOO) && defined(BAR)
int b;
#elif !defined(FOO) && !defined(BAR)
int c;
#else
int d;
#endif

