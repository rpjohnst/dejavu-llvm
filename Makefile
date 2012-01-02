# toolchain configuration

CXX := clang++
CPPFLAGS := -Wall -Wextra -g
CXXFLAGS :=
LDFLAGS :=
LDLIBS :=

# llvm configuration
CXXFLAGS += $(shell llvm-config --cxxflags)
LDFLAGS += $(shell llvm-config --ldflags)
LDLIBS += $(shell llvm-config --libs core native scalaropts ipo)

# gather source files

SOURCES := $(shell find -name '*.cc')
OBJECTS := $(SOURCES:.cc=.o)
DEPENDENCIES := $(OBJECTS:.o=.d)

# build targets

.PHONY: all clean

all: compiler

clean:
	$(RM) compiler $(OBJECTS) $(DEPENDENCIES)

# rules and dependencies

compiler: $(OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)

# don't put %.d as a dependency or things get built twice
%.o: %.cc
	$(CXX) -std=c++11 -c -Iinclude -MMD -MP -MT '$*.o $*.d' $(CPPFLAGS) $(CXXFLAGS) -o $*.o $<

ifneq ($(MAKECMDGOALS),clean)
-include $(DEPENDENCIES)
endif
