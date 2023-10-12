#!/bin/bash

set -e

DIR=$(dirname $0)
source $DIR/../setup-vars.sh

SOURCESDIR="$vamos_sources_SRCDIR"
NUM=$1
test -z $NUM && NUM=10000

DRRUN="$DRIO_BUILD/bin64/drrun"
if [ ! -x $DRRUN ]; then
	echo "Could not find drrun"
	exit 1
fi

$DRRUN -root $DRIO_BUILD \
	-opt_cleancall 2 -opt_speed\
	-c $SOURCESDIR/src/drregex/libdrregex.so\
	/primes1 prime '#([0-9]+): ([0-9]+)' ii --\
	$DIR/primes $NUM &

$DRRUN -root $DRIO_BUILD \
	-opt_cleancall 2 -opt_speed\
	-c $SOURCESDIR/src/drregex/libdrregex.so\
	/primes2 prime '#([0-9]+): ([0-9]+)' ii --\
	$DIR/primes-bad $NUM 10&

wait
wait
