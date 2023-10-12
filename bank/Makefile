include ../Makefile.config

CPPFLAGS=
CFLAGS=-g -O3
VLCC=python3 ${vamos_compiler_SRCDIR}/compiler/main.py
GENCC=${vamos_compiler_SRCDIR}/gen/compile.sh
SHAMON_LIB=${vamos_buffers_libraries_DIR}/libvamos-buffers-lib.so  # for dependency

all: monitor-vamos monitor-tessla bank interact

intmap.o: ${vamos_compiler_SRCDIR}/compiler/cfiles/intmap.cpp
	$(CXX) -c $<

$(VLCC):  # empty, just a depenency for monitor
$(GENCC):
$(SHAMON_LIB):

interact: interact.c
	$(CC) $^ $(CFLAGS) $(CPPFLAGS) -o $@ -g

monitor.c: bankmonitor.vl  $(VLCC)
	$(VLCC) bankmonitor.vl -o monitor.c
	-clang-format -i monitor.c

monitor-vamos: monitor.c $(GENCC) intmap.o $(SHAMON_LIB)
	$(GENCC) monitor.c intmap.o -lstdc++
	mv monitor monitor-vamos

monitor-tessla.c: bankmonitor-tessla.vl  $(VLCC)
	$(VLCC) bankmonitor-tessla.vl -o monitor-tessla.c
	-clang-format -i monitor-tessla.c

monitor-tessla: monitor-tessla.c $(GENCC) intmap.o $(SHAMON_LIB)
	$(GENCC) monitor-tessla.c intmap.o -lstdc++
	mv monitor monitor-tessla

bank.o : bank.c bank.h
	$(CC) -c $< $(CFLAGS) $(CPPFLAGS) -o $@

client.o : client.c bank.h common.h
	$(CC) -c $< $(CFLAGS) $(CPPFLAGS) -o $@

bank: client.o bank.o
	$(CC) $^ $(CFLAGS) $(CPPFLAGS) -o $@ -lpthread

experiments: interact
	./experiments.sh

clean:
	-rm *.o monitor-vamos monitor-tessla bank interact *.o

.PHONY: clean all experiments
