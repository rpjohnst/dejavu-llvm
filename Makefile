# toolchain configuration

CXX := clang++
CPPFLAGS := -Wall -Wextra -g
CXXFLAGS :=
LDFLAGS :=
LDLIBS :=

# llvm configuration

LLVM_DIR := ../llvm-install-dbg/bin
LLVM_CONFIG := $(LLVM_DIR)/llvm-config

CXXFLAGS += $(shell $(LLVM_CONFIG) --cxxflags)
LDFLAGS += $(shell $(LLVM_CONFIG) --ldflags)
LDLIBS += $(shell $(LLVM_CONFIG) --libs core)

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
