#!/bin/sh

rm -f tmp_test.c
rm -f tests.h
rm -f tmp_*.c

(echo '#include "tests.h"'
echo 'int main()'
echo '{'

for i in *-*.c
do
	n=`echo $i | sed 's/\(.*\)-.*\.c/\1/'`
	sed s/main/main_$n/ < $i > tmp_$n.c
	echo "int main_$n();" >> tests.h
	echo "main_$n();"
	
done

echo 'return 0;'
echo '}'
) > tmp_test.c

