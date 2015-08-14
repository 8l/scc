#!/bin/sh

out=/tmp/$$.out
chk=/tmp/$$.chk

trap "rm -f $out $chk" EXIT INT QUIT HUP

for i in *.c
do
	awk '
	BEGIN {
		out="'$out'";chk="'$chk'"
		system("rm -f " out " " chk)
	}
	/^name:/ {
		printf "Running %s ", $2
	}
	/^output:$/ {
		copyon=1
	}
	/^\*/  {
		copyon=0
	}
	copyon==1 && !/^output:$/  {
		print $0 >> chk
	}
	END {
		system("../cc1 -w '$i' > " out " 2>&1")
		print system("cmp -s " out " " chk) ? "[FAILED]" : "[OK]"
	}' $i
done
