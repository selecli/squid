#!/bin/bash

# add CFLAGS to Make.properties
CFLAGS_TAG="CFLAGS = "
echo $CFLAGS_TAG
CFLAGS=`grep "^$CFLAGS_TAG" ./Make.properties`
echo $CFLAGS
if [ "$CFLAGS" = "$CFLAGS_TAG" ];  then
	echo 'repace CFLAGS'
	CFLAGS=`grep "^$CFLAGS_TAG" ./squid/Makefile`
	echo $CFLAGS
#	sed -i 's/CFLAGS = /'"$CFLAGS"'/g' Make.properties
fi
