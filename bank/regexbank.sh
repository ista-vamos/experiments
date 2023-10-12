#!/bin/bash

set -e

NUM=$1
shift

source ../setup-vars.sh

DRRUN="$DRIO_BUILD/bin64/drrun"
if [ ! -x $DRRUN ]; then
	echo "Could not find drrun"
	exit 1
fi

DRRUN="$DRRUN -root $DRIO_BUILD/\
	-c $vamos_sources_SRCDIR/src/drregex/libdrregex-mt.so"

MONITOR=$(dirname $0)/monitor-vamos
if [ $(basename "$0") == "regexbank-dump.sh" ]; then
	MONITOR=$vamos_buffers_LIBRARIES_DIR_core/../../monitors/monitor-generic
fi

rm -f /tmp/fifo{A,B}
rm -f /dev/shm/bank.{stdin,stdout}

mkfifo /tmp/fifo{A,B}

python3 inputs.py $NUM > inputs.last.txt

# we can start it a bit before as it is waiting for the connection
echo "-- Starting the monitor --"
$MONITOR Out:regex:/bank.stdout In:regex:/bank.stdin&
MON_PID=$!

$DRRUN -t /bank \
balance "\s*Current balance on Account ([0-9]+):\s*" i \
depositTo "\s*Deposit to Account ([0-9]+)" i \
withdraw "\s*Withdraw from Account ([0-9]+)" i \
transfer "\s*Transfer from Account ([0-9]+) to Account ([0-9]+)" ii \
depositSuccess "^Deposit successful!" $'' \
depositFail "^Deposit amount must be positive!" $'' \
withdrawSuccess "^Withdrawal successful!" $'' \
withdrawFail "^Withdrawal failed!" $'' \
transferSuccess "^Transfer successful!" $'' \
selectedAccount "\s*Selected account: ([0-9]+).*" i \
numOut "^\s*([0-9]+)\s*$" i \
logout "Logged out!" $'' \
-stdin \
numIn "^\s*([0-9]+)\s*$" i \
otherIn ".*[^\s].*" $'' -- $(dirname $0)/bank $@ < /tmp/fifoA  > /tmp/fifoB &
BANK_PID=$!

echo "-- Starting interact --"
cat /tmp/fifoB | ./interact inputs.last.txt interact.log >/tmp/fifoA &

#/usr/lib/linux-intel-iotg-5.15-tools-5.15.0-1016/perf record --call-graph lbr -p $BANK_PID
wait $MON_PID

rm -f /tmp/fifo{A,B}

cat interact.log
