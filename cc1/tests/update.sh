#!/bin/sh

update()
{
	(echo '/^output/+;/^\*\//-c'
	../cc1 -I./ -w $1 2>&1
	printf ".\nw\n") | ed -s $1
}


case $# in
1)
	update $1
	exit
	;;
*)
	echo "usage: update.sh [test]" >&2
	exit 1
	;;
esac


for i in *.c
do
	update $i
done
