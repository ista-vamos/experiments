include ../Makefile.config

ifndef BUFSIZE
$(error BUFSIZE variable not set, use `make <target> BUFSIZE=N`)
endif

ifndef TAU
$(error TAU variable not set, use `make <target> TAU=N`)
endif

ifndef DELTA
$(error DELTA variable not set, use `make <target> DELTA=N`)
endif



CPPFLAGS=-g
CFLAGS=-g #-O3

VLCC=python3 ${vamos_compiler_SRCDIR}/compiler/main.py
GENCC=${vamos_compiler_SRCDIR}/gen/compile.sh
COMPILER_CFILES=${vamos_compiler_SRCDIR}/compiler/cfiles/

all: monitor


compiler_utils.o: ${COMPILER_CFILES}/compiler_utils.c
	$(CC) -I${vamos_buffers_INCLUDE_DIRS} -c $< -o compiler_utils.o

intmap.o: ${COMPILER_CFILES}/intmap.cpp
	$(CXX) -c $< -o intmap.o

monitor-gen.c: monitor.vl
	$(VLCC) monitor.vl -b $(BUFSIZE) -f 100 -o monitor-gen.c
	-clang-format -i monitor-gen.c

monitor.c: monitor.c.in
	sed "s/@WAYLAND_BUFFER_SIZE@/$(BUFSIZE)/" monitor.c.in > monitor.c
	sed -i "s/@LIBINPUT_BUFFER_SIZE@/$(BUFSIZE)/" monitor.c
	sed -i "s/@TAU@/$(TAU)/" monitor.c
	sed -i "s/@DELTA@/$(DELTA)/" monitor.c

monitor.cpp: monitor.cpp.in
	sed "s/@TAU@/$(TAU)/" monitor.cpp.in > monitor.cpp
	sed -i "s/@DELTA@/$(DELTA)/" monitor.cpp

# always re-generate the input files because the parameters may have changed
.PHONY: monitor.c.in monitor.cpp.in

monitor.o: monitor.cpp
	$(CXX) -c $(CXXFLAGS) monitor.cpp

monitor: monitor.c monitor.o $(GENCC) compiler_utils.o
	$(GENCC) monitor.c monitor.o compiler_utils.o -lstdc++

# only compile, for testing
compile: compiler_utils.o
	$(GENCC) monitor.c compiler_utils.o

