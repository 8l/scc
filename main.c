
#include <stddef.h>

#include "cc.h"
#include "tokens.h"
#include "syntax.h"

extern void open_file(const char *file);
extern void run(struct node *np);
struct user_opt options;



int
main(int argc, char *argv[])
{
	struct node *np;

	init_keywords();
	open_file(NULL);
	for (next(); yytoken != EOFTOK; run(np))
		np = extdecl();

	return 0;
}
