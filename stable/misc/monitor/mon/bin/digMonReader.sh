#! /bin/bash

# Chinacache SquidDevTeam
# $Id$

test -r /monitor/pool/digMon.pool&&cat /monitor/pool/digMon.pool \
||echo -e "-1\n-1\n-1 "
