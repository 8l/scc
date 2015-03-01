#!/bin/sh

case `uname` in
Plan9)
	CFLAGS="-D_SUSV2_SOURCE -DNBOOL"
	export CFLAGS
	;;
esac

make $@

