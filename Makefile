.PHONY: run all indent clean check coverage

CPLUSPLUS?=g++
CPPFLAGS=-std=c++11 -Wall -W
ifeq ($(NDEBUG),1)
CPPFLAGS+= -O3
else
CPPFLAGS+= -g
endif
LDFLAGS=-pthread

PWD=$(shell pwd)
SRC=$(wildcard *.cc)
BIN=$(SRC:%.cc=.noshit/%)

run: .noshit/sockets
	.noshit/sockets

clean:
	rm -rf .noshit core

.noshit/%: %.cc
	mkdir -p .noshit
	${CPLUSPLUS} ${CPPFLAGS} -o $@ $< ${LDFLAGS}

indent:
	(find . -name "*.cc" ; find . -name "*.h") | grep -v "/.noshit/" | xargs clang-format-3.5 -i
