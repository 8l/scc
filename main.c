
#include <stddef.h>

#include "tokens.h"
#include "syntax.h"

extern void open_file(const char *file);
extern void init_lex();





int main(int argc, char *argv[])
{
	init_lex();
	open_file(NULL);
	for (next(); yytoken != EOFTOK; decl())
		/* nothing */;

	return 0;
}
