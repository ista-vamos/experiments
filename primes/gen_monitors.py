import os, sys
from subprocess import run, TimeoutExpired

sys.path.append('..')
import config

arbiter_buffer_sizes = [4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048]
# generate monitors with different buffer size

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
COMPILER_PATH = f"{config.vamos_compiler_DIR}/compiler/vamosc.py"


# generate oject file of intmap
run(["clang++", "-c", f"{config.vamos_compiler_DIR}/compiler/cfiles/intmap.cpp" ], check=True)

# compile shamon programs
for buffsize in arbiter_buffer_sizes:
    print(f"Generating monitor with arbiter buffer size {buffsize}")
    run(["python3", COMPILER_PATH, f"{CURRENT_PATH}/programs/primes_{buffsize}.txt",
         "-freq", "10000000", "-e", f"{CURRENT_PATH}/programs/monitor_{buffsize}",
         "-l", f"{CURRENT_PATH}/intmap.o", #"-l" , f"{CURRENT_PATH}/compiler_utils.o",
         "-Lstdc++" 
         ],
        check=True)


# compile empty monitor
# COMPILE_SCRIPT= f"{config.vamos_compiler_DIR}/gen/compile.sh"
# for buffsize in arbiter_buffer_sizes:
#     # compile c files
#     new_env = os.environ.copy()
#     new_env["ARBITER_BUFSIZE"] = str(buffsize)
#     run(["bash", COMPILE_SCRIPT, f"{CURRENT_PATH}/emptymonitor.c", f"-DSHMBUF_ARBITER_BUFSIZE={buffsize}"],
#         check=True, env=new_env)
# 
#     # move exec to /primes/programs
#     run(["mv", "monitor", f"{CURRENT_PATH}/programs/empty_monitor{buffsize}" ], check=True)
# 

