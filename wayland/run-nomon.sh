#!/bin/bash
#
TOUCHPAD=/dev/input/event12

weston --fullscreen --width 1920 --height 1080&
WESTON_PID=$!

export WAYLAND_DISPLAY=wayland-1

sleep 2
weston-terminal&
T1_PID=$!
weston-terminal&
T2_PID=$!
sleep 1

evemu-play $TOUCHPAD < events1.txt

kill -INT $T1_PID
kill -INT $T2_PID
kill -INT $WESTON_PID
