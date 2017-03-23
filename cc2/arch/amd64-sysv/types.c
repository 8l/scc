/* See LICENSE file for copyright and license details. */
static char sccsid[] = "@(#) ./cc2/arch/amd64-sysv/types.c";

#include "../../../inc/cc.h"
#include "../../cc2.h"


Type int8type = {
	.flags  = SIGNF | INTF,
	.size   = 1,
	.align  = 1
};

Type int16type = {
	.flags  = SIGNF | INTF,
	.size   = 2,
	.align  = 2
};

Type int32type = {
	.flags  = SIGNF | INTF,
	.size   = 4,
	.align  = 4
};

Type int64type = {
	.flags  = SIGNF | INTF,
	.size   = 8,
	.align  = 8
};

Type uint8type = {
	.flags  = INTF,
	.size   = 1,
	.align  = 1
};

Type uint16type = {
	.flags  = INTF,
	.size   = 2,
	.align  = 2
};

Type uint32type = {
	.flags  = INTF,
	.size   = 4,
	.align  = 4
};

Type uint64type = {
	.flags  = INTF,
	.size   = 8,
	.align  = 2
};

Type ptrtype = {
	.flags  = INTF,
	.size   = 8,
	.align  = 8
};

Type booltype = {
	.flags  = INTF,
	.size   = 1,
	.align  = 1
};

Type float32type = {
	.flags  = FLOATF,
	.size   = 4,
	.align  = 4
};

Type float64type = {
	.flags  = FLOATF,
	.size   = 8,
	.align  = 8
};

Type float80type = {
	.flags  = FLOATF,
	.size   = 16,
	.align  = 16
};

Type voidtype = {
	.size = 0,
	.align = 0
};

Type arg_type = {
	.size = 24,
	.align = 8
};
