#!/bin/bash

set -e

cd $(dirname $0)
source ../setup-vars.sh

if [ ! -f "$1" ]; then
	echo "First argument is the file with events"
	exit 1
fi

TOUCHPAD=/dev/input/event13
WIDTH=1920
HEIGHT=1080
APP=weston-terminal

weston --fullscreen --width $WIDTH --height $HEIGHT&
WESTON_PID=$!

sleep 1

export WAYLAND_DISPLAY=wayland-1

$vamos_sources_SRCDIR/src/wldbg/wldbg-source &>wldbg.log &
$vamos_sources_SRCDIR/src/libinput/vsrc-libinput --shmkey /libinput &>libinput.log &
LIBINPUT_PID=$!

sleep 1
./monitor Clients:generic:/vamos-wldbg Libinput:generic:/libinput &> monitor.log &
MONITOR_PID=$!

sleep 1
$APP&
T1_PID=$!
$APP&
T2_PID=$!

xdotool mousemove --sync $((WIDTH/2)) $((HEIGHT/2))
evemu-play $TOUCHPAD < $1

kill -INT $T1_PID
kill -INT $T2_PID
kill -INT $WESTON_PID

wait $MONITOR_PID
kill -KILL $LIBINPUT_PID || true
