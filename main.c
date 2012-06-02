
#include <stddef.h>

extern void open_file(const char *file);
extern void init_lex();
extern void next();
extern void stmt();



int main(int argc, char *argv[])
{
	init_lex();
	open_file(NULL);
	next();
	compound();

	return 0;
}
