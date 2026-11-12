
CXX=g++
CXXFLAGS=-Wall -g
CC=$(CXX)
DEP=$(CXX) -MM

OBJS=frontend.o LinuxSetBaudRate.o SerialPort.o LinuxPort.o

all: frontend


frontend: $(OBJS)
	$(CXX) -o frontend $(OBJS)

deps:
	$(DEP) $(patsubst %.o,%.cpp,$(OBJS)) > Make.deps

clean:
	-/bin/rm $(wildcard *.o) frontend

include Make.deps
