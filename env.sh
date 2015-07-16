#!/bin/sh

case `uname` in
Plan9)
	CFLAGS="-D_SUSV2_SOURCE -DNBOOL"
	export CFLAGS
	;;
*)
	case x$CC in
	xc99)
		;;
	x|xgcc)
		CC=gcc
		CFLAGS=-std=c99
		export CFLAGS CC
		;;
	*)
		echo You need a c99 compiler for this program 2>&1
		exit
		;;
	esac
	;;
esac
