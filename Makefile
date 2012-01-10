# top-level commands

.PHONY: all
all: gmlc runtime.bc

.PHONY: clean
clean:
	$(RM) gmlc $(gmlc_OBJECTS) $(gmlc_DEPENDS) runtime.bc $(runtime_OBJECTS) $(runtime_DEPENDS)

# toolchain configuration

CXX := clang++
CXXFLAGS := -Wall -Wextra -g

DEPFLAGS = -MMD -MP -MT $@ -MT $*.d

# build the compiler

gmlc_SOURCES := main.cc $(shell find compiler system -name '*.cc')
gmlc_OBJECTS := $(gmlc_SOURCES:.cc=.o)
gmlc_DEPENDS := $(gmlc_SOURCES:.cc=.d)

gmlc_CPPFLAGS := $(shell llvm-config --cppflags)
gmlc_LDFLAGS := $(shell llvm-config --ldflags)
gmlc_LDLIBS := $(shell llvm-config --libs core native scalaropts ipo)

gmlc: $(gmlc_OBJECTS)
	$(CXX) $(gmlc_LDFLAGS) -o $@ $^ $(gmlc_LDLIBS)

%.o: %.cc
	$(CXX) -c -std=c++11 -Iinclude $(DEPFLAGS) $(CXXFLAGS) $(gmlc_CXXFLAGS) $(gmlc_CPPFLAGS) -o $@ $<

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
-include $(gmlc_DEPENDS) $(runtime_DEPENDS)
endif
