# top-level commands

TARGETS := dejavu.jar dejavu.so runtime.bc

.PHONY: all
all: $(TARGETS)

.PHONY: clean
clean:
	$(RM) $(TARGETS) $(interface_OBJECTS) $(interface_DEPENDS) $(library_OBJECTS) $(library_DEPENDS) $(runtime_OBJECTS) $(runtime_DEPENDS)
	(cd plugin && ant clean)

# toolchain configuration

CXX := clang++
CXXFLAGS := -Wall -Wextra -g

# build the interface

interface_SOURCES := $(wildcard plugin/*.i)
interface_OBJECTS := $(wildcard plugin/*_wrap.*) $(wildcard plugin/org/dejavu/backend/*.java)
interface_DEPENDS := $(interface_SOURCES:.i=.d)

%_wrap.cc: %.i | plugin/org/dejavu/backend
	swig -Wall -MMD -MF $*.d -O -Iinclude -c++ -java -package org.dejavu.backend -outdir plugin/org/dejavu/backend -o $@ $<

plugin/org/dejavu/backend:
	mkdir $@

# build the plugin

plugin_SOURCES := $(shell find plugin -name '*.java')

dejavu.jar: $(plugin_SOURCES) plugin/dejavu_wrap.cc
	cd plugin && ant

# build the library

library_SOURCES := $(shell find compiler linker system plugin -name '*.cc') plugin/dejavu_wrap.cc
library_OBJECTS := $(library_SOURCES:.cc=.o)
library_DEPENDS := $(library_SOURCES:.cc=.d)

library_CXXFLAGS := -fpic
library_CPPFLAGS := $(shell llvm-config --cppflags) -I$(JAVA_HOME)/include{,/linux}
library_LDFLAGS := -shared $(shell llvm-config --ldflags)
library_LDLIBS := $(shell llvm-config --libs core native scalaropts ipo linker bitreader bitwriter)

dejavu.so: $(library_OBJECTS)
	$(CXX) $(library_LDFLAGS) -o $@ $^ $(library_LDLIBS)

%.o: %.cc
	$(CXX) -c -std=c++11 -Iinclude -MMD -MP $(CXXFLAGS) $(library_CXXFLAGS) $(library_CPPFLAGS) -o $@ $<

# build the runtime

runtime_SOURCES := $(shell find runtime -name '*.cc')
runtime_OBJECTS := $(runtime_SOURCES:.cc=.bc)
runtime_DEPENDS := $(runtime_SOURCES:.cc=.d)

runtime.bc: $(runtime_OBJECTS)
	llvm-link -o $@ $^

%.bc: %.cc
	$(CXX) -c -emit-llvm -std=c++11 -Iinclude $(DEPFLAGS) $(CXXFLAGS) -o $@ $<

# include dependencies

ifeq ($(filter clean, $(MAKECMDGOALS)),)
-include $(interface_DEPENDS) $(library_DEPENDS) $(runtime_DEPENDS)
endif
