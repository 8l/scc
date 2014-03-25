
#ifndef CODE_H_
#define CODE_H_

struct symbol;

extern void emitsym(struct symbol *sym),  emitfun(struct symbol *sym),
	emitframe(struct symbol *sym), emitret(struct symbol *sym);

#endif
