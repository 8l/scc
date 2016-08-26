#!/bin/sh

trap 'tabs -8;rm -f a.out' 0 1 2 3 15
tabs 40
ulimit -c 0

for i in $@
do
	printf "%s\t" $i
	rm -f a.out
	(scc -m qbe "$i" && ./a.out) 2>/dev/null && echo [OK] || echo [FAILED]
done
