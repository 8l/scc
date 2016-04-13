/*
name: TEST044
description: Test of corner cases in #if
error:
test044.c:15: warning: division by 0
test044.c:19: warning: division by 0
test044.c:23: warning: division by 0
test044.c:29: error: parameter of #if is not an integer constant expression
test044.c:30: error: #error  3 != (1,2,3)
output:
*/

/* These should be accepted */

  #if 0 != (0 && (0/0))
    #error 0 != (0 && (0/0))
  #endif

  #if 1 != (-1 || (0/0))
    #error 1 != (-1 || (0/0))
  #endif

  #if 3 != (-1 ? 3 : (0/0))
    #error 3 != (-1 ? 3 : (0/0))
  #endif

/* This is invalid code (it is a constraint violation) */

  #if 3 != (1,2,3)
    #error 3 != (1,2,3)
  #endif

