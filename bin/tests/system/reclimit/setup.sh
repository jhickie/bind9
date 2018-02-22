#!/bin/sh
#
# Copyright (C) 2014, 2016, 2018  Internet Systems Consortium, Inc. ("ISC")
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

SYSTEMTESTTOP=..
. $SYSTEMTESTTOP/conf.sh

$SHELL clean.sh
copy_setports ns1/named.conf.in ns1/named.conf
copy_setports ns3/named1.conf.in ns3/named.conf
