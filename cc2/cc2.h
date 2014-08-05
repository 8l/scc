
typedef struct {
	union {
		struct {
			char type;
			char storage;
		} v;
		struct {
			short addr;
		} l;
	} u;
} Symbol;

typedef struct node {
	char op;
	char type;
	int8_t sethi;
	int8_t addrtype;
	union {
		short id;
		int imm;
	} u;
	struct node *left, *right;
} Node;

