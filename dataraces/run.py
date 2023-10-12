#!/usr/bin/env python3

import sys
from subprocess import run, Popen, PIPE, DEVNULL, TimeoutExpired
from os.path import dirname, abspath, basename, isfile

from signal import SIGKILL, SIGTERM
from os import killpg, setpgid, environ


from randomharness import gen_harness

sys.path.append(abspath(f"{dirname(__file__)}/.."))
import config

DIR = abspath(dirname(sys.argv[0]))
env_TIMEOUT = environ.get("DATARACES_TIMEOUT")
TIMEOUT = 20 if env_TIMEOUT is None else int(env_TIMEOUT)
env_REPEAT = environ.get("DATARACES_REPEAT_RUN")
REPEAT_NUM = 3 if env_REPEAT is None else int(env_REPEAT)
CSVFILE="results.csv"

opt = "opt"
link = "llvm-link"

# we expect output like this:
# 0.01user 0.00system 0:00.03elapsed 48%CPU (0avgtext+0avgdata 25772maxresident)k
# 0inputs+0outputs (0major+1946minor)pagefaults 0swaps
timecmd = "/bin/time"

env_LOG = environ.get("LOG")
logpath = env_LOG or f"{DIR}/log.txt"
logfile = None

def cmd(args, quiet=True, quiet_cmd=False):
    if not quiet_cmd:
        print("> ", " ".join(args))
    if quiet:
        global logfile
        if logfile is None:
            logfile = open(logpath, "a")
        print("="*70, file=logfile)
        print("> ", " ".join(args), file=logfile)
        run(args, check=True, stdout=logfile, stderr=logfile)
        print("="*70, file=logfile)
    else:
        run(args, check=True)

def parse_time(tm):
    parts = tm.split(":")
    assert len(parts) == 2
    return 60 * int(parts[0]) + float(parts[1])


def compile_file(infile):
    harness = environ.get("DATARACES_HARNESS")
    if harness is None:
        harness = f"{DIR}/harness.tmp.c"
        # generate harness
        with open(harness, "w") as out:
            gen_harness(out, 1024)

    # compile TSAN
    cmd(
        [
            "clang",
            infile,
            harness,
            f"{DIR}/svcomp_atomic.c",
            "-g",
            "-fsanitize=thread",
            "-pthread",
            "-fgnu89-inline",
            "-O3",
            "-o",
            "a.tsan.out",
        ]
    )

    # compile HELGRIND
    cmd(
        [
            "gcc",
            infile,
            harness,
            f"{DIR}/svcomp_atomic.c",
            "-g",
            "-fgnu89-inline",
            "-O3",
            "-pthread",
            "-o",
            "a.helgrind.out",
        ]
    )

    # compile VAMOS
    cmd(
        [
            f"{config.vamos_sources_SRCDIR}/src/tsan/compile.py",
            infile,
            "-noinst",
            harness,
            f"{DIR}/svcomp_atomic.c",
            "-o",
            "a.vamos.out",
        ]
    )


def run_once():
    result = {}

    def set_pgroup():
        setpgid(0, 0)

    # ---- run TSAN
    result.setdefault("tsan", {})["races"] = 0

    try:
        proc = Popen([timecmd, "./a.tsan.out"], stderr=PIPE, stdout=DEVNULL, preexec_fn=set_pgroup)
        _, err = proc.communicate(timeout=TIMEOUT)
        for line in err.splitlines():
            if b"WARNING: ThreadSanitizer: data race" in line:
                result["tsan"]["races"] += 1
            if b"elapsed" in line and b"maxresident" in line:
                words = line.split()
                result["tsan"]["usertime"] = float(words[0][:-4])
                result["tsan"]["systime"] = float(words[1][:-6])
                result["tsan"]["time"] = parse_time(words[2][:-7].decode("ascii"))
                result["tsan"]["maxmem"] = int(
                    words[5][0 : words[5].find(b"maxresident")]
                )
    except TimeoutExpired:
        result["tsan"]["races"] = "TO"
        if proc.poll() is None:
            killpg(proc.pid, SIGTERM)
            killpg(proc.pid, SIGKILL)

    # --- run HELGRIND
    result.setdefault("helgrind", {})["races"] = 0
    try:
        proc = Popen(
            [timecmd, "valgrind", "--tool=helgrind", "./a.helgrind.out"],
            stderr=PIPE,
            stdout=DEVNULL,
            preexec_fn=set_pgroup
        )
        _, err = proc.communicate(timeout=TIMEOUT)
        for line in err.splitlines():
            if b"Possible data race" in line:
                result["helgrind"]["races"] += 1
            if b"elapsed" in line and b"maxresident" in line:
                words = line.split()
                result["helgrind"]["usertime"] = float(words[0][:-4])
                result["helgrind"]["systime"] = float(words[1][:-6])
                result["helgrind"]["time"] = parse_time(words[2][:-7].decode("ascii"))
                result["helgrind"]["maxmem"] = int(
                    words[5][0 : words[5].find(b"maxresident")]
                )
    except TimeoutExpired:
        result["helgrind"]["races"] = "TO"
        if proc.poll() is None:
            killpg(proc.pid, SIGTERM)
            killpg(proc.pid, SIGKILL)

    # --- run VAMOS
    result.setdefault("vamos", {})["races"] = 0
    try:
        cmd(["rm", "-f", "/dev/shm/vrd*"], quiet_cmd=True)
        proc = Popen([timecmd, "./a.vamos.out"],
                     stderr=PIPE, stdout=DEVNULL, preexec_fn=set_pgroup)
        mon  = Popen([timecmd, "./monitor", "Program:generic:/vrd"],
                     stderr=PIPE, stdout=DEVNULL, preexec_fn=set_pgroup)
        _, err_proc = proc.communicate(timeout=TIMEOUT)
        _, err_mon = mon.communicate(timeout=TIMEOUT)

        for line in err_mon.splitlines():
            if line.startswith(b"Found data race"):
                result["vamos"]["races"] += 1
            elif b"Dropped" in line and b"holes" in line:
                words = line.split()
                result["vamos"]["dropped"] = int(words[1])
                result["vamos"]["holes"] = int(words[4])
            elif b"elapsed" in line and b"maxresident" in line:
                words = line.split()
                result["vamos"]["maxmem-mon"] = int(
                    words[5][0 : words[5].find(b"maxresident")]
                )
        for line in err_proc.splitlines():
            if b"elapsed" in line and b"maxresident" in line:
                words = line.split()
                result["vamos"]["usertime"] = float(words[0][:-4])
                result["vamos"]["systime"] = float(words[1][:-6])
                result["vamos"]["time"] = parse_time(words[2][:-7].decode("ascii"))
                result["vamos"]["maxmem"] = int(
                    words[5][0 : words[5].find(b"maxresident")]
                )
            if b"info: number of emitted events" in line:
                result["vamos"]["eventsnum"] = int(line.split()[5])
    except TimeoutExpired:
        result["vamos"]["races"] = "TO"
    finally:
        if proc.poll() is None:
            killpg(proc.pid, SIGTERM)
            killpg(proc.pid, SIGKILL)
        if mon.poll() is None:
            killpg(mon.pid, SIGTERM)
            killpg(mon.pid, SIGKILL)

    # the output is:
    # benchmark,
    # tsan-{races, usertime, systime, time, maxmem},
    # helgrind-{races, usertime, systime, time, maxmem},
    # vamos-{races, usertime, systime, time, maxmem}, vamos-{eventsnum, dropped, holes}
    ret = []
    for tool in "tsan", "helgrind", "vamos":
        res = result[tool]
        ret += [
            res.get(k) for k in ("races", "usertime", "systime", "time", "maxmem")
        ]
        if tool == "vamos":
            ret += [res.get(k) for k in ("maxmem-mon", "eventsnum", "dropped", "holes")]

    return ret


def get_stats(results):
    assert len(results) > 0
    retlen = len(results[0])
    ret = [0] * retlen

    for i in range(retlen):
        for result in results:
            ret[i] += result[i]

    return [x / len(results) for x in ret]


def run_rep(infile, csvfile):
    compile_file(infile)
    results = []
    i = 0
    n = 0
    csvstream = None
    if csvfile:
        if isfile(csvfile):
            print("Appending to the old CSV output file: ", csvfile)
            csvstream = open(csvfile, "a")
        else:
            print("Creating a new CSV output file: ", csvfile)
            csvstream = open(csvfile, "w")
            # write the header
            print("benchmark,tsan-races,tsan-usertime,tsan-systime,tsan-time,tsan-maxmem,"\
                  "helgrind-races,helgrind-usertime,helgrind-systime,helgrind-time,helgrind-maxmem,"\
                  "vamos-races,vamos-usertime,vamos-systime,vamos-time,vamos-maxmem,vamos-maxmem-mon,"\
                  "vamos-eventsnum,vamos-dropped,vamos-holes",
                  file=csvstream)

    while True:
        i += 1
        if i > 2 * REPEAT_NUM:
            raise RuntimeError("Failed measuring")
        result = run_once()
        #print("\033[0;33mRESULT", basename(infile), ",".join((str(r) for r in result)), "\033[0m")
        print("\033[0;32mraces/time/mem:", basename(infile),
              f"tsan: ({result[0]}, {result[3]}, {result[4]})",
              f"helg: ({result[5]}, {result[8]}, {result[9]})",
              f"vams: ({result[10]}, {result[13]}, {result[14]}+{result[15]})",
              "\033[0m")
        if csvstream:
            print(basename(infile),",", ",".join((str(r) for r in result)), file=csvstream)

        sys.stdout.flush()
        assert len(result) == 19
        #results.append(result)
        n += 1
        if n == REPEAT_NUM:
            break

    #stats = get_stats(results)
    #print("RESULT", basename(infile), ",".join((f"{r:.2f}" for r in stats)))
    #return results
    if csvfile:
        csvstream.close()

def main(argv):
    if not (2 <= len(argv) <= 3):
        print("Usage: ./run.py file.c [output.csv]")
        exit(1)

    run_rep(argv[1], argv[2] if len(argv) > 2 else None)

    global logfile
    if logfile:
        logfile.close()

if __name__ == "__main__":
    main(sys.argv)
    exit(0)
