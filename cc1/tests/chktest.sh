#!/bin/sh

out=/tmp/out
chk=/tmp/chk

#trap "rm -f $out $chk" EXIT INT QUIT

for i in *.c
do
	rm -f $out $chk
	awk '
	/^name:/                  {printf "Running %s ", $2}
	/^output:$/               {copyon=1}
	/^\*/                     {copyon=0}
	copyon==1 && !/^output:$/ {print $0 >> "'$chk'"}
	' $i

	../cc1 $i > $out 2>&1
  	if cmp $out $chk >/dev/null 2>&1
  	then
   		echo [OK]
  	else
  		echo [FAILED]
 	fi
done
