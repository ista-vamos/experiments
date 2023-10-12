#!/usr/bin/env python3
# coding: utf-8

from tempfile import mkdtemp
from shutil import rmtree
from os import chdir
from measure import *
from sys import argv
from run_common import *

import sys
sys.path.append('..')
import config

NUM="10000"
if len(argv) > 2:
    BS = argv[1]
    ABS = argv[2]
    if len(argv) > 3:
        NUM=argv[3]
else:
    print("args: shm-buffer-size arbiter-buffer-size [number of primes]")
    print("shm-buffer-size is the compiled (!) size, it does not set the size.")
    print("arbiter-buffer-size is will set the size of the arbiter buffer in the monitor.")
    exit(1)

open_log()
open_csvlog(BS, ABS, NUM)

lprint(f"Enumerating primes up to {NUM}th prime...")
#lprint(f"Taking average of {repeat_num()} runs...\n")

EMPTY_MONITOR_PATH = f"{PRIMESPATH}/programs/empty_monitor{ABS}"
MONITOR_PATH = f"{PRIMESPATH}/programs/monitor{ABS}"

###############################################################################

c_native = ParseTime(withwaitstats=False)
measure("'C primes' native",
        [Command(f"{PRIMESPATH}/primes", NUM).withparser(c_native)])
c_native.report('c-native')

c_drio = ParseTime(withwaitstats=False)
measure("'C primes' DynamoRIO (no monitor)",
        [Command(*DRIO, "--", f"{PRIMESPATH}/primes", NUM).withparser(c_drio)])
c_drio.report('c-drio')
###############################################################################

WORKINGDIR = mkdtemp(prefix="midmon.", dir="/tmp")
chdir(WORKINGDIR)
lprint(f"-- Working directory is {WORKINGDIR} --")

# create temporary names for SHM files
primes1 = rand_shm_name('primes1')
primes2 = rand_shm_name('primes2')

lprint(f"--  Using empty monitor: {EMPTY_MONITOR_PATH} --")

dm_drio_consume_time_c1 = ParseTime()
dm_drio_consume_time_c2 = ParseTime()
measure("'Empty monitor C/C for primes' DynamoRIO sources",
        [Command(*DRIO, "-c", f"{config.vamos_sources_SRCDIR}/src/drregex/libdrregex.so",
                 primes1, "Prime", "#([0-9]+): ([0-9]+)", "ii", "--",
                 f"{PRIMESPATH}/primes", NUM).withparser(dm_drio_consume_time_c1),
         Command(*DRIO, "-c", f"{config.vamos_sources_SRCDIR}/src/drregex/libdrregex.so",
                 primes2, "Prime", "#([0-9]+): ([0-9]+)", "ii", "--",
                 f"{PRIMESPATH}/primes", NUM).withparser(dm_drio_consume_time_c2)],
        [Command(EMPTY_MONITOR_PATH, f"Left:drregex:{primes1}", f"Right:drregex:{primes2}",
                 stdout=PIPE)])
dm_drio_consume_time_c1.report('c-c-empty', msg="C (1) program")
dm_drio_consume_time_c2.report('c-c-empty', msg="C (2) program")

###############################################################################

log(f"-- Removing working directory {WORKINGDIR} --")
chdir("/")
rmtree(WORKINGDIR)

WORKINGDIR = mkdtemp(prefix="midmon.", dir="/tmp")
chdir(WORKINGDIR)
lprint(f"-- Working directory is {WORKINGDIR} --")

lprint(f"-- Using differential monitor: {MONITOR_PATH} --")

###############################################################################

dm_drio_time_c1 = ParseTime()
dm_drio_time_c2 = ParseTime()
dm_drio_stats2 = ParseStats()
measure("'Differential monitor C/C for primes' DynamoRIO sources",
        [Command(*DRIO, "-c", f"{config.vamos_sources_SRCDIR}/src/drregex/libdrregex.so",
                 primes1, "Prime", "#([0-9]+): ([0-9]+)", "ii", "--",
                 f"{PRIMESPATH}/primes", NUM).withparser(dm_drio_time_c1),
         Command(*DRIO, "-c", f"{config.vamos_sources_SRCDIR}/src/drregex/libdrregex.so",
                 primes2, "Prime", "#([0-9]+): ([0-9]+)", "ii", "--",
                 f"{PRIMESPATH}/primes", NUM).withparser(dm_drio_time_c2)],
        [Command("/bin/time", "-f", "Wall-time: %e", MONITOR_PATH, f"P_0:drregex:{primes1}", f"P_1:drregex:{primes2}",
                 stdout=PIPE).withparser(dm_drio_stats2)])
dm_drio_time_c1.report('c-c-dm', msg="C (1) program")
dm_drio_time_c2.report('c-c-dm', msg="C (2) program")
dm_drio_stats2.report('c-c-dm-stats')
assert dm_drio_stats2.errs == 0, "Had errors in non-error traces"

dm_drio_bad_time_c1 = ParseTime()
dm_drio_bad_time_c2 = ParseTime()
dm_drio_bad_stats2 = ParseStats()
measure("'Differential monitor C/C for primes' DynamoRIO sources, 10% errors",
        [Command(*DRIO, "-c", f"{config.vamos_sources_SRCDIR}/src/drregex/libdrregex.so",
                 primes1, "Prime", "#([0-9]+): ([0-9]+)", "ii", "--",
                 f"{PRIMESPATH}/primes", NUM).withparser(dm_drio_bad_time_c1),
         Command(*DRIO, "-c", f"{config.vamos_sources_SRCDIR}/src/drregex/libdrregex.so",
                 primes2, "Prime", "#([0-9]+): ([0-9]+)", "ii", "--",
                 f"{PRIMESPATH}/primes-bad", NUM, str(int(NUM)/10)).withparser(dm_drio_bad_time_c2)],
         [Command("/bin/time", "-f", "Wall-time: %e", MONITOR_PATH, f"P_0:drregex:{primes1}",
                 f"P_1:drregex:{primes2}", stdout=PIPE).withparser(dm_drio_bad_stats2)])
dm_drio_bad_time_c1.report('c-c-dm-errs-good', msg="C (1) program (good)")
dm_drio_bad_time_c2.report('c-c-dm-errs-bad', msg="C (2) program (bad)")
dm_drio_bad_stats2.report('c-c-dm-errs-stats')

log(f"-- Removing working directory {WORKINGDIR} --")
chdir("/")
rmtree(WORKINGDIR)

log("-- This run is DONE! --")

log("-- Closing logs --")
close_log()
close_csvlog()


