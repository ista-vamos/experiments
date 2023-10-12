import os, sys
from subprocess import run, TimeoutExpired

sys.path.append('..')
import config

arbiter_buffer_sizes = [4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048]
# generate shamon programs with different buffer size

if not os.path.exists("./programs"):
    os.mkdir("./programs")

for buffsize in arbiter_buffer_sizes:
        file = open("./primes.txt.in", "r")
        outfile = open(f"./programs/primes_{buffsize}.txt", "w")
        for line in file:
            if "@BUFSIZE" in line:
                line = line.replace("@BUFSIZE", str(buffsize))
            if "@PARAM2" in line:
                line = line.replace("@PARAM2", str(int(round(param2*buffsize,0))) )
            outfile.write(line)
        outfile.close()
        file.close()

CURRENT_PATH = os.getcwd()
COMPILER_PATH = f"{config.vamos_compiler_SRCDIR}/compiler/main.py"

# compile shamon programs into c programs
for buffsize in arbiter_buffer_sizes:
    run(["python3", COMPILER_PATH, f"{CURRENT_PATH}/programs/primes_{buffsize}.txt", "-freq", "10000000", "-o", f"{CURRENT_PATH}/programs/monitor_{buffsize}.c"], check=True)


# generate oject file of intmap
run(["clang++", "-c", f"{config.vamos_compiler_SRCDIR}/compiler/cfiles/intmap.cpp" ], check=True)

COMPILE_SCRIPT= f"{config.vamos_compiler_SRCDIR}/gen/compile.sh"
for buffsize in arbiter_buffer_sizes:
        # compile c files
        run(["bash", COMPILE_SCRIPT, f"{CURRENT_PATH}/programs/monitor_{buffsize}.c", "intmap.o", "compiler_utils.o", "-lstdc++" ], check=True)

        # move exec to /primes/programs
        run(["mv", "monitor", f"{CURRENT_PATH}/programs/monitor{buffsize}" ], check=True)


# compile empty monitor

for buffsize in arbiter_buffer_sizes:
    # compile c files
    new_env = os.environ.copy()
    new_env["ARBITER_BUFSIZE"] = str(buffsize)
    run(["bash", COMPILE_SCRIPT, f"{CURRENT_PATH}/emptymonitor.c", f"-DSHMBUF_ARBITER_BUFSIZE={buffsize}"],
        check=True, env=new_env)

    # move exec to /primes/programs
    run(["mv", "monitor", f"{CURRENT_PATH}/programs/empty_monitor{buffsize}" ], check=True)


