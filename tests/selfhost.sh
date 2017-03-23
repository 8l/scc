#! /bin/sh

# A script to self host whatever object files that we can as a regression test.
# One day it can be replaced with:
# make install && make clean && make CC=scc install

set -e
set -u

unset SCCEXECPATH

selfhostedobj=\
"lib/xstrdup.o
 lib/xmalloc.o
 lib/xcalloc.o
 lib/xrealloc.o"
#lib/newitem.o
#lib/debug.o
#lib/die.o
#driver/posix/scc.o
#cc1/error.o
#cc1/stmt.o
#cc1/init.o
#cc1/arch/qbe/arch.o
#cc1/fold.o
#cc1/types.o
#cc1/builtin.o
#cc1/cpp.o
#cc1/symbol.o
#cc1/lex.o
#cc1/decl.o
#cc1/main.o
#cc1/code.o
#cc1/expr.o
#cc2/arch/qbe/cgen.o
#cc2/arch/qbe/types.o
#cc2/arch/qbe/optm.o
#cc2/arch/qbe/code.o
#cc2/peep.o
#cc2/parser.o
#cc2/node.o
#cc2/symbol.o
#cc2/optm.o
#cc2/main.o
#cc2/code.o"

if ! test -d ./cc1
then
	echo "run this script from the root of the scc repository."
	exit 1
fi

boostrapdir="$(pwd)/_bootstrap"
rm -rf "$boostrapdir"
mkdir "$boostrapdir"

make clean
make PREFIX="$boostrapdir" install
export PATH="$boostrapdir/bin:$PATH"

rm lib/libcc.a bin/scc bin/cc*
rm $selfhostedobj

make CC=scc tests
