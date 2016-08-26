/* See LICENSE file for copyright and license details. */

/*
name: TEST063
description: Test a comment that goes beyond of the end of an included file
error:
test063.h:9: error: unterminated comment
test063.c:12: error: #endif expected
output:
*/

#include "test063.h"
