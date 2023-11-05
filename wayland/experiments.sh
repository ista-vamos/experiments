#!/bin/bash

CSV=results.csv

for BUFSIZE in 4 8 16 32 64 128 256 512; do
for TAU in 10 50 100 200; do
for DELTA in 1 10 50 100; do
for EVENTS in events1.txt; do
	make BUFSIZE=$BUFSIZE TAU=$TAU DELTA=$DELTA
	rm monitor.log
	./run.sh $EVENTS
	ERRS=$(grep "ERROR" monitor.log | wc -l)
	echo "$EVENTS $BUFSIZE $TAU $DELTA $ERRS" >> $CSV
done
done
done
done
