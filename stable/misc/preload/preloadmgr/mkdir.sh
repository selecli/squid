#!/bin/bash
read value1 value2
rm -fr /tmp/$value1/LocalObject/*
for x in `seq 0 127`;
	do 
		mkdir -p /tmp/$value1/LocalObject/$x;
done
