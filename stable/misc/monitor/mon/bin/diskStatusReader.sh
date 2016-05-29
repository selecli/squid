#!/bin/bash
Date=$(date +%s)
Modified=$(stat -c %Y /monitor/pool/diskStatus.pool 2>/dev/null)
Interval=$(( 300 + ${Modified:-0} - $Date))
[ "$Interval" -ge 0 ]&&cat /monitor/pool/diskStatus.pool||{ echo -1;echo -1 ; }
