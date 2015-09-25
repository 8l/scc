#!/bin/sh

for i in *.c
do
	(echo '/^output/+;/^\*\//-c'
	../cc1 -w $i 2>&1
	printf ".\nw\n") | ed -s $i
done
