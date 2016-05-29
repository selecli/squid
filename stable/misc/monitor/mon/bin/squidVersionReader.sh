#!/bin/bash

if [ -f /monitor/pool/squidVersion.pool ];then
        cat /monitor/pool/squidVersion.pool
else
        echo "-1"
        echo "-1"
fi
