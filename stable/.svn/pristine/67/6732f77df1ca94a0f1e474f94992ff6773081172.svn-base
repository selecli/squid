all: diskSpaceMon diskSpeedMon monitor netBandwidthMon netStatusMon squidLogTransMon timeNtpChkMon
.PHONY: all

diskSpaceMon: diskSpaceMon.o
	gcc -o ../bin/diskSpaceMon diskSpaceMon.o

diskSpeedMon: diskSpeedMon.o
	gcc -o ../bin/diskSpeedMon diskSpeedMon.o

monitor: monitor.o
	gcc -o ../bin/monitor monitor.o

netBandwidthMon: netBandwidthMon.o
	gcc -o ../bin/netBandwidthMon netBandwidthMon.o

netStatusMon: netStatusMon.o
	gcc -o ../bin/netStatusMon netStatusMon.o

squidLogTransMon: squidLogTransMon.o
	gcc -o ../bin/squidLogTransMon squidLogTransMon.o

timeNtpChkMon: timeNtpChkMon.o
	gcc -o ../bin/timeNtpChkMon timeNtpChkMon.o

diskSpaceMon.o: diskSpaceMon.c
	gcc -c diskSpaceMon.c

diskSpeedMon.o: diskSpeedMon.c
	gcc -c diskSpeedMon.c

monitor.o: monitor.c
	gcc -c monitor.c

netBandwidthMon.o: netBandwidthMon.c
	gcc -c netBandwidthMon.c

netStatusMon.o: netStatusMon.c
	gcc -c netStatusMon.c

squidLogTransMon.o: squidLogTransMon.c
	gcc -c squidLogTransMon.c

timeNtpChkMon.o: timeNtpChkMon.c
	gcc -c timeNtpChkMon.c

.PHONY: cleanall cleanobj
cleanall: cleanobj
	-rm -f ../bin/diskSpaceMon ../bin/diskSpeedMon ../bin/monitor ../bin/netBandwidthMon \
               ../bin/netStatusMon ../bin/squidLogTransMon ../bin/timeNtpChkMon
cleanobj:
	rm -f *.o
