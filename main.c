
#include <stddef.h>
#include <stdint.h>

#include "cc.h"
#include "tokens.h"
#include "syntax.h"

extern void open_file(const char *file);
struct user_opt options;

int
main(int argc, char *argv[])
{
	init_keywords();
	open_file(NULL);
	for (next(); yytoken != EOFTOK; extdecl());
		/* nothing */;

	return 0;
}
