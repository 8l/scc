
#include <stddef.h>

#include "cc.h"
#include "tokens.h"
#include "syntax.h"

extern void open_file(const char *file);
extern void init_lex();
struct user_opt user_opt;




int main(int argc, char *argv[])
{
	init_lex();
	open_file(NULL);
	for (next(); yytoken != EOFTOK; decl())
		/* nothing */;

	return 0;
}
