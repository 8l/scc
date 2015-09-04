#!/bin/sh

out=/tmp/$$.out
chk=/tmp/$$.chk
err=test.log

trap "rm -f $out $chk" EXIT INT QUIT HUP
rm -f $err

for i in *.c
do
	awk '
	BEGIN {
		out="'$out'"
		chk="'$chk'"
		err="'$err'"
		test="'$i'"
		system("rm -f " out " " chk)
	}
	/^name:/ {
		printf "Running %s ", $2
	}
	/^output:$/ {
		copyon=1
	}
	/^\*\//  {
		copyon=0
	}
	copyon==1 && !/^output:$/  {
		print $0 >> chk
	}
	END {
		system("../cc1 -w " test " > " out " 2>&1")
		cmd="diff -c " chk " " out " >> " err
		print test >> err
		print system(cmd) ? "[FAILED]" : "[OK]"
	}' $i
done
