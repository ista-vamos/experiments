from measure import *
from csv import writer as csvwriter
from sys import argv
from tempfile import mktemp
from os.path import dirname, basename, abspath, join as pathjoin, isdir

import sys
sys.path.append('..')
import config

SELFPATH=abspath(dirname(__file__))
DRIOPATH=abspath(config.get_drio_builddir())

DRRUN=f"{DRIOPATH}/bin64/drrun"
DRIO=[DRRUN, "-root",  f"{DRIOPATH}/",
             "-opt_cleancall", "2", "-opt_speed"]
PRIMESPATH=f"{SELFPATH}/"

csvlog = None
csvlogf = None

# check if the monitor is a known monitor

def open_csvlog(BS, ABS, NUM):
    csv_name = f"times-{BS}-{ABS}-{NUM}.csv"

    global csvlogf
    csvlogf = open(csv_name, 'a')

    global csvlog
    csvlog = csvwriter(csvlogf)

def close_csvlog():
    csvlogf.close()

class ParseTime:
    def __init__(self, dbg=False, withwaitstats=True):
        self.waiting = []
        self.times = []
        self.time = 0
        self.waited = 0
        self.dbg = dbg
        self.withwaitstats = withwaitstats

    def parse(self, out, serr):
        if self.dbg:
            if out:
               for line in out.splitlines():
                   lprint(f"[stdout] {line}", color=GRAY, end='')
            if serr:
                for line in serr.splitlines():
                    lprint(f"[stderr] {line}", color=GRAY, end='')

        foundwaited = False
        foundtime = False
        for line in serr.splitlines():
            if line.startswith(b'time:'):
                parts = line.split()
                assert len(parts) == 3, parts
                f = float(parts[1])
                assert f > 0
                self.times.append(f)
                self.time += f
                foundtime = True
                #return
            if b'busy waited' in line:
                parts = line.split()
                assert len(parts) == 10, parts
                c = int(parts[8])
                self.waited += c
                self.waiting.append(c)
                foundwaited = True

        if not foundtime or\
            (self.withwaitstats and not foundwaited):
            log("-- ERROR while parsing stderr for time info:",
                "-- ERROR while parsing time (see /tmp/log.txt)--")
            log(serr)
            raise RuntimeError("Did not find right values")

    def report(self, key, msg=None):
        assert self.time > 0, f"Invalid time: {self.time}"
        if msg:
            lprint(msg, end=' ', color="\033[0;32m")
        val = self.time / repeat_num()
        lprint(f"Avg CPU time ({repeat_num()} measurements): {val} seconds.",
               color="\033[0;32m")
        if self.waiting:
            for t, c in zip(self.times, self.waiting):
                csvlog.writerow([key, t, c])
        else:
            assert not self.withwaitstats
            for t in self.times:
                csvlog.writerow([key, t])
        return val

class ParseStats:
    def __init__(self):
        self.stats = []
        self.dl, self.dr = None, None
        self.sl, self.sr = None, None
        self.il, self.ir = None, None
        self.pl, self.pr = None, None
        self.errs = None
        self.time = None

    def parse(self, out, serr):
        return self._parse_mon_man(out, serr)

    def report(self, key, msg=None):
        return self._report_mon_man(key, msg)

    def _parse_mon_man(self, out, serr):
        errs = 0
        dl, dr = None, None
        pl, pr = None, None
        sl, sr = None, None
        total_left = None
        total_right = None
        tm = None
        for line in out.splitlines():
            if line.startswith(b'ERROR'):
                errs += 1
            # LEFT : processed 100000 events (21397 compared, 52293 dropped (in 348 holes), 26310 skipped)
            if line.startswith(b'LEFT'):
                parts = line.split()
                assert len(parts) == 13, parts
                total_left = int(parts[2])
                pl = int(parts[4][1:])
                dl = int(parts[6])
                sl = int(parts[11])
            if line.startswith(b'RIGHT'):
                parts = line.split()
                assert len(parts) == 13, parts
                total_right = int(parts[2])
                pr = int(parts[4][1:])
                dr = int(parts[6])
                sr = int(parts[11])
        if dl is None or sl is None or pl is None or\
           dl + sl + pl == 0 or dl + sl + pl != dr + sr + pr:
            log(out)
            lprint(f"left: {total_left} {(dl, sl, pl)}, right:{total_right} {(dr, sr, pr)}, errs: {errs}")
            lprint("-- ERROR while parsing monitor output (see /tmp/log.txt)--")
            lprint("Did not find right values", color=RED)
            
            raise RuntimeError("Did not find right values")

        for line in serr.splitlines():
            if b'Wall-time' in line:
                parts = line.split()
                assert len(parts) == 2
                tm = float(parts[1])
        if tm is None:
            raise RuntimeError("No time found in stderr")

        self.stats.append(((dl, sl, pl),(dr, sr, pr), errs, tm))
        self.dl, self.dr = dl, dr
        self.sl, self.sr = sl, sr
        self.pl, self.pr = pl, pr
        self.errs = errs


    def _report_mon_man(self, key, msg=None):
        assert self.stats
        if msg:
            lprint(msg, end=' ', color="\033[0;32m")
        # FIXME: average is not a good statistics here
        dl = sum(self.stats[i][0][0] for i in range(len(self.stats)))
        dr = sum(self.stats[i][1][0] for i in range(len(self.stats)))
        sl = sum(self.stats[i][0][1] for i in range(len(self.stats)))
        sr = sum(self.stats[i][1][1] for i in range(len(self.stats)))
        pl = sum(self.stats[i][0][2] for i in range(len(self.stats)))
        pr = sum(self.stats[i][1][2] for i in range(len(self.stats)))
        errs = sum(self.stats[i][2] for i in range(len(self.stats)))
        N = repeat_num()
        lprint(\
f"""
Average of {N} measurements:

Left dropped: {dl / N}
Right dropped: {dr / N}
Left skipped: {sl / N}
Right skipped: {sr / N}
Left processed: {pl / N}
Right processed: {pr / N}
Detected errors: {errs/ N}""",
               color=GREEN)

        for Sl, Sr, E, T in self.stats:
            csvlog.writerow([key, *Sl, *Sr, E, T])
        return ((dl, sl, pl),(dr, sr, pr), errs, T)

    def _report_mon_gen(self, key, msg=None):
        assert self.stats
        if msg:
            lprint(msg, end=' ', color="\033[0;32m")
        # FIXME: average is not a good statistics here
        dl = sum(self.stats[i][0][0] for i in range(len(self.stats)))
        dr = sum(self.stats[i][1][0] for i in range(len(self.stats)))
        sl = sum(self.stats[i][0][1] for i in range(len(self.stats)))
        sr = sum(self.stats[i][1][1] for i in range(len(self.stats)))
        il = sum(self.stats[i][0][2] for i in range(len(self.stats)))
        ir = sum(self.stats[i][1][2] for i in range(len(self.stats)))
        pl = sum(self.stats[i][0][3] for i in range(len(self.stats)))
        pr = sum(self.stats[i][1][3] for i in range(len(self.stats)))
        errs = sum(self.stats[i][2] for i in range(len(self.stats)))
        N = repeat_num()
        lprint(\
f"""
Average of {N} measurements:

Left dropped: {dl / N}
Right dropped: {dr / N}
Left skipped: {sl / N}
Right skipped: {sr / N}
Left ignored: {il / N}
Right ignored: {ir / N}
Left processed: {pl / N}
Right processed: {pr / N}
Detected errors: {errs/ N}""",
               color=GREEN)

        for Sl, Sr, E in self.stats:
            csvlog.writerow([key, *Sl, *Sr, E])
        return ((dl, sl, il, pl),(dr, sr, ir, pr), errs)

#####################################################################

def rand_shm_name(key):
    if key[0] == '/':
        key = key[1:]
    return mktemp(prefix=f"/{key}.")
