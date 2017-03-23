#!/bin/sh

trap 'tabs -8;rm -f a.out' 0 1 2 3 15
tabs 40
ulimit -c 0
rm -f test.log

while read i state
do
	echo $i >>test.log
	printf "%s\t" $i
	printf "%s" $state
	rm -f a.out
	(scc -Isysinclude $CFLAGS "$i" && ./a.out) 2>test.log &&
		echo [OK] || echo [FAILED]
done
