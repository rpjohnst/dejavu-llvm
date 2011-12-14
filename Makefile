# toolchain configuration

CXX := clang++
CPPFLAGS := -Wall -Wextra -g
CXXFLAGS :=
LDFLAGS :=

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

%.o %.d: %.cc
	$(CXX) -c -Iinclude -MMD -MP -MT '$*.o $*.d' $(CPPFLAGS) $(CXXFLAGS) -o $*.o $<

ifneq ($(MAKECMDGOALS),clean)
-include $(DEPENDENCIES)
endif
