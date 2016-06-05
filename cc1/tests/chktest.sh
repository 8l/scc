#!/bin/sh
# See LICENSE file for copyright and license details.

out=/tmp/$$.out
err=/tmp/$$.err
chk=/tmp/$$.chk
tst=/tmp/$$.tst

trap "rm -f $out $chk $err $tst" EXIT INT QUIT HUP
rm -f test.log

for i
do
	rm -f $chk
	awk '/^name:/            {printf "Running %s ", $2}
	     /^error:$/          {copyon=1; next}
	     /^TODO/             {printf "[DISABLED]"}
	     /^output:$/         {next}
	     /^\*\//             {copyon=0; next}
	     copyon==1           {print > "'$chk'"}' $i

	../cc1 -I. -w $i > $out 2>$err
	echo $i >> test.log
	cat $err $out > $tst
	if diff -c $chk $tst >> test.log
	then
		echo [OK]
	else
		echo [FAILED]
	fi
done
