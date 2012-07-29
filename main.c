
#include <stddef.h>

#include "cc.h"
#include "tokens.h"
#include "syntax.h"

extern void open_file(const char *file);
struct user_opt user_opt;




int main(int argc, char *argv[])
{
	init_keywords();
	open_file(NULL);
	for (next(); yytoken != EOFTOK; decl())
		/* nothing */;

	return 0;
}
