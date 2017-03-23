#!/bin/sh
# See LICENSE file for copyright and license details.

for i
do
	(echo '/^PATTERN/+;/^\./-c'
	 scc $CFLAGS -w -c $i 2>&1
	 printf ".\nw\n"
	 echo w) |
	ed -s $i
done
