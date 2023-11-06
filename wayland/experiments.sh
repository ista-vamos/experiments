#!/bin/bash

CSV=results.csv
mkdir -p logs

for BUFSIZE in 3; do
for TAU in 10 30 50; do
for DELTA in 10 30 50 70 100; do
for EVENTS in events2.txt events3.txt; do
	make BUFSIZE=$BUFSIZE TAU=$TAU DELTA=$DELTA
	rm monitor.log
	./run.sh $EVENTS
	cp monitor.log logs/$EVENTS-$BUFSIZE-$TAU-$DELTA-monitor.log

	NUMS=$(python parse_log.py monitor.log)
	echo -e "\e[0;31m$EVENTS,$BUFSIZE,$TAU,$DELTA,$NUMS\e[0m"
	echo "$EVENTS,$BUFSIZE,$TAU,$DELTA,$NUMS" >> $CSV
done
done
done
done
