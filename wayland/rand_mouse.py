import sys
from subprocess import run
from random import randint

def diff(x, nx):
    if nx > x:
        return 1
    return -1 if nx < x else 0

def move(x, y, new_x, new_y):
    while new_x != x and new_y != y:
        dx = diff(x, new_x)
        dy = diff(y, new_y)
        x += dx
        y += dy
        # evemu-event /dev/input/event11 --type EV_REL --code REL_X --value 10
        cmd = ["evemu-event", "/dev/input/event11",
               "--type", "EV_REL",
               "--code", "REL_Y",
               "--value", str(dy)]
        #print(">  ", " ".join(cmd))
        run(cmd, check=True)
        cmd = ["evemu-event", "/dev/input/event11",
               "--type", "EV_REL",
               "--code", "REL_X",
               "--value", str(dx)]
        #print(">  ", " ".join(cmd))
        run(cmd, check=True)



W = int(sys.argv[1])
H = int(sys.argv[2])
N = int(sys.argv[3])

x = int(W/2)
y = int(H/2)

while N > 0:
    N -= 1

    while True:
        dx = randint(-100, 100)
        dy = randint(-100, 100)

        tmpx = x + dx
        tmpy = y + dy

        if (tmpx < 0) or (tmpy < 0):
            continue
        if (tmpx == x and tmpy == y):
            continue

        if tmpx < W and tmpy < H:
            move(x, y, tmpx, tmpy)
            x, y = tmpx, tmpy
            break
