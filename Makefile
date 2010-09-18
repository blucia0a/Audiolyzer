BLDTYPE ?= pin

PIN_HOME ?= $(HOME)/pin
PIN_HEADER_DIR = $(PIN_HOME)/source/tools

AUDIERE_HOME ?= $(HOME)/cvsandbox/Apps/audiere
AUDIERE_INCLUDE=$(AUDIERE_HOME)/include
AUDIERE_LIB=$(AUDIERE_HOME)/lib

ifeq ($(BLDTYPE),pin)
	include $(PIN_HEADER_DIR)/makefile.gnu.config
endif

OS=gnu
ifeq ($(OS),gnu)
	CXX=g++
	CC=gcc
	CXXFLAGS += -DGNU
endif

#Options
CXXFLAGS += -O0 -fPIC -g
DBG = -g

#Source Files
SRCS = Audiolyzer.cpp


CXXFLAGS += -I$(AUDIERE_INCLUDE)
LDFLAGS += -L$(AUDIERE_LIB) -laudiere
OBJS = $(SRCS:%.cpp=%.o)

all: $(SD)

## build rules

%.o : %.cpp
	$(CXX) -c $(CXXFLAGS) $(PIN_CXXFLAGS) -o $@ $<

Audiolyzer: $(OBJS)
	$(CXX) -g $(PIN_LDFLAGS) $(LDFLAGS) -o $@ $+ $(PIN_LIBS)

run: $(PINTOOL)
	$(PIN_HOME)/Bin/pin -mt -t $(TOOL) -c domains.conf -- $(CMDLINE)

## cleaning
clean:
	-rm -f *.lo *.o $(TOOL) *.out *.tested *.failed *.d

nd:
	gcc -O0 -lpthread -g -o Nondeterminate Nondeterminate.c

-include *.d

