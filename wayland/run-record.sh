#!/bin/bash
#
TOUCHPAD=/dev/input/event12

WIDTH=1920
HEIGHT=1080
weston --fullscreen --width $WIDTH --height $HEIGHT&
WESTON_PID=$!

export WAYLAND_DISPLAY=wayland-1

sleep 2
weston-terminal&
T1_PID=$!
weston-terminal&
T2_PID=$!
sleep 1

xdotool mousemove --sync $((WIDTH/2)) $((HEIGHT/2))
evemu-record /dev/input/event12 $1&
EVEMU_PID=$!
sleep 1m


kill -INT $EVEMU_PID
kill -INT $T1_PID
kill -INT $T2_PID
kill -INT $WESTON_PID
