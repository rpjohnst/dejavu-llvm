# toolchain configuration

CXX := g++
CPPFLAGS := -Wall -Wextra
CXXFLAGS := -std=c++0x
LDFLAGS :=

# gather source files

SOURCES := $(shell find -name \*.cc)
OBJECTS := $(SOURCES:.cc=.o)
DEPENDENCIES := $(OBJECTS:.o=.d)

# build targets

.PHONY: all clean

all: compiler

clean:
	rm -f compiler $(OBJECTS) $(DEPENDENCIES)

# rules and dependencies

compiler: $(OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $^

-include $(DEPENDENCIES)

# dependency targets must be separate to avoid over-building

%.o: %.cc
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $*.o $<

%.d: %.cc
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MM -MP -MT '$*.o $*.d' -o $@ $<
