#!/bin/sh
#
# Copyright (C) 2015, 2016  Internet Systems Consortium, Inc. ("ISC")
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

SYSTEMTESTTOP=..
. $SYSTEMTESTTOP/conf.sh

. ../getopts.sh

USAGE="$0: [-xD]"
DEBUG=
while getopts "xD" c; do
    case $c in
	x) set -x; DEBUG=-x;;
        D) TEST_DNSRPS="-D";;
	N) NOCLEAN=set;;
	*) echo "$USAGE" 1>&2; exit 1;;
    esac
done
shift `expr $OPTIND - 1 || true`
if test "$#" -ne 0; then
    echo "$USAGE" 1>&2
    exit 1
fi

[ ${NOCLEAN:-unset} = unset ] && $SHELL clean.sh $DEBUG

$PERL testgen.pl
$SEDPORTS < ns1/named.conf.in > ns1/named.conf
echo "${port}" > ns1/named.port
$SEDPORTS < ns2/named.conf.header.in > ns2/named.conf.header
echo "${port}" > ns2/named.port
cp -f ns2/named.default.conf ns2/named.conf
$SEDPORTS < ns3/named1.conf.in > ns3/named.conf
echo "${port}" > ns3/named.port
$SEDPORTS < ns4/named.conf.in > ns4/named.conf
echo "${port}" > ns4/named.port
$SEDPORTS < ans5/ans.pl.in > ans5/ans.pl

# decide whether to test DNSRPS
$SHELL ../rpz/ckdnsrps.sh $TEST_DNSRPS $DEBUG
test -z "`grep 'dnsrps-enable yes' dnsrps.conf`" && TEST_DNSRPS=

CWD=`pwd`
cat <<EOF >dnsrpzd.conf
PID-FILE $CWD/dnsrpzd.pid;

include $CWD/dnsrpzd-license-cur.conf

zone "policy" { type master; file "`pwd`/ns3/policy.db"; };
EOF
sed -n -e 's/^ *//' -e "/zone.*.*master/s@file \"@&$CWD/ns2/@p" ns2/*.conf \
    >>dnsrpzd.conf

# Run dnsrpzd to get the license and prime the static policy zones
if test -n "$TEST_DNSRPS"; then
    DNSRPZD="`../rpz/dnsrps -p`"
    "$DNSRPZD" -D./dnsrpzd.rpzf -S./dnsrpzd.sock -C./dnsrpzd.conf \
		-w 0 -dddd -L stdout >./dnsrpzd.run 2>&1
fi
