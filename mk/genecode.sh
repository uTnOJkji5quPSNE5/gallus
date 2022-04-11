#!/bin/sh

if test ! -x ./configure -o ! -d ./mk -o ! -r ./mk/ecode.def; then
    exit 1
fi

prfx=gallus
if test $# -ge 1; then
    prfx="${1}"
fi
cprfx=`echo ${prfx} | tr '[a-z]' '[A-Z]'`

h=./src/include/${prfx}_ecode.h
if test -r ./mk/ecode2h.awk; then
    nkf -Lu -d ./mk/ecode.def | \
	awk -f ./mk/ecode2h.awk -v prfx=${cprfx}_RESULT_ | \
	nkf -Lw -c > "${h}"
    st=$?
    if test ${st} -ne 0; then
	exit ${st}
    fi
fi

c=./src/lib/ecode.c
if test -r ./mk/ecode2c.awk; then
    nkf -Lu -d ./mk/ecode.def | \
	awk -F'"' -f ./mk/ecode2c.awk | \
	nkf -Lw -c > "${c}"
    st=$?
    if test ${st} -ne 0; then
	exit ${st}
    fi
fi

exit 0


