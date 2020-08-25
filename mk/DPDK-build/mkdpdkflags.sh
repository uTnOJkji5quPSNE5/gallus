#!/bin/sh

mkfile=./Makefile.dpdk
if test ! -r "${mkfile}"; then
    exit 1
fi

if test -z "${MAKE}"; then
    MAKE=make
fi

DPDK_LIBS=`${MAKE} --no-print-directory -f "${mkfile}" dpdk_libs | 
		sed -e 's:--whole-archive:-Wl,--whole-archive:g' \
		-e 's:--no-whole-archive:-Wl,--no-whole-archive:g'`
echo "DPDK_LIBS	=	${DPDK_LIBS}"
echo

_ldflags=''
_l=`${MAKE} --no-print-directory -f "${mkfile}" dpdk_ldflags`
for i in ${_l}; do
    case ${i} in
	-L*)	dir=`echo ${i} | sed 's:^-L::'`
		if test -d "${dir}"; then
		    _ldflags="${_ldflags} ${i}"
		fi;;
	-*|--*)	_ldflags="${_ldflags} -Wl,${i}";;
    esac
done
echo "DPDK_LDFLAGS	=	${_ldflags}"	

_gotOption=''
_cppflags=''
_cflags=''
_c=`${MAKE} --no-print-directory -f "${mkfile}" dpdk_cflags`
for i in ${_c}; do
    if test "x${gotOption}" = "x"; then
	case ${i} in
	    -include)	gotOption=${i};;
	    -D*|-U*)	_cppflags="${_cppflags} ${i}";;
	    -I*)	dir=`echo ${i} | sed 's:^-I::'`
			if test -d "${dir}"; then
			    _cppflags="${_cppflags} ${i}"
			fi;;
	    -fPIC|-pthread)	true;;
	    *)		_cflags="${_cflags} ${i}";;
	esac
    else
	if test "x${gotOPtion}"="x-include"; then
	    if test -r "${i}"; then
		_cppflags="${_cppflags} -include ${i}"
	    fi
	else
	    _cflags="${_cflags} ${i}"
	fi
	gotOption=''
    fi
done
echo "DPDK_CPPFLAGS	=	${_cppflags}"
echo
echo "DPDK_CFLAGS	=	${_cflags}"
echo

exit 0
