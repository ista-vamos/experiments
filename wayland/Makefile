include ../Makefile.config

ifndef BUFSIZE
#$(error BUFSIZE variable not set, use `make <target> BUFSIZE=N`)
BUFSIZE=1024
endif

CPPFLAGS=
CFLAGS= #-O3

VLCC=python3 ${vamos_compiler_SRCDIR}/compiler/main.py
GENCC=${vamos_compiler_SRCDIR}/gen/compile.sh
COMPILER_CFILES=${vamos_compiler_SRCDIR}/compiler/cfiles/

all: monitor


compiler_utils.o: ${COMPILER_CFILES}/compiler_utils.c
	$(CC) -I${vamos_buffers_INCLUDE_DIRS} -c $< -o compiler_utils.o

intmap.o: ${COMPILER_CFILES}/intmap.cpp
	$(CXX) -c $< -o intmap.o

monitor-gen.c: monitor.vl
	$(VLCC) monitor.vl -b $(BUFSIZE) -f 2000 -o monitor-gen.c
	-clang-format -i monitor-gen.c

monitor: monitor.c $(GENCC) compiler_utils.o
	$(GENCC) monitor.c compiler_utils.o

# only compile, for testing
compile: compiler_utils.o
	$(GENCC) monitor.c compiler_utils.o

