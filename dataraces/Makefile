include ../Makefile.config

ARBITER_BUFFER:=2048

CXX=clang++
CXXFLAGS=
CFLAGS=-fPIC -std=c11
CPPFLAGS=-D_POSIX_C_SOURCE=200809L
LDFLAGS=-lpthread

ifdef DEBUG
CFLAGS += -g -O0
CXXFLAGS += -g -O0
CPPFLAGS += -DDBGBUF
else # DEBUG
ifeq ($(vamos_buffers_BUILD_TYPE), Debug)
CFLAGS +=-g -O0
CXXFLAGS +=-g -O0
else
CFLAGS +=-g3 -O3 -flto -fno-fat-lto-objects
CXXFLAGS +=-g3 -O3 -flto -fno-fat-lto-objects
CPPFLAGS += -DNDEBUG
endif
endif

MON_COMPILER_PY=$(vamos_compiler_SRCDIR)/compiler/main.py
MON_COMPILER=$(vamos_compiler_SRCDIR)/gen/compile.sh

all: monitor

monitor.o: monitor.cpp
	$(CXX) -I$(vamos_buffers_INCLUDE_DIRS) $(CXXFLAGS) $(CPPFLAGS) $(MONITOR_CPP_FLAGS) monitor.cpp -std=c++17 -c

monitor.c: monitor.txt clean
	python3 $(MON_COMPILER_PY) monitor.txt -b $(ARBITER_BUFFER) -o monitor.c
	-clang-format -i monitor.c

compiler_utils.o: $(vamos_compiler_SRCDIR)/compiler/cfiles/compiler_utils.c
	$(CC) -g -c $<  -I$(vamos_buffers_INCLUDE_DIRS) -o $@

monitor: monitor.c monitor.o compiler_utils.o
	$(MON_COMPILER) monitor.c monitor.o -I$(vamos_compiler_SRCDIR) -lstdc++ compiler_utils.o

run: monitor
	./monitor Program:generic:/vrd

experiments: monitor
	./experiments.sh

clean:
	-rm monitor.c *.bc *.o vgcore*

.PHONY: all clean


