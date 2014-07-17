
#include <stddef.h>

#include <cc.h>
#include <sizes.h>

extern void parse(void);

void
esyntax(void)
{
	die("incorrect intermediate file");
}

int
main(void)
{
	parse();
}

