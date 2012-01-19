# top-level commands

.PHONY: all
all: dejavu.jar dejavu.so runtime.bc

.PHONY: clean
clean:
	$(RM) dejavu.jar $(plugin_OBJECTS) dejavu.so $(library_OBJECTS) $(library_DEPENDS) runtime.bc $(runtime_OBJECTS) $(runtime_DEPENDS)

# toolchain configuration

CXX := clang++
CXXFLAGS := -Wall -Wextra -g

JAVAC := javac

JAR := jar

DEPFLAGS = -MMD -MP -MT $@

# build the plugin

plugin_SOURCES := $(shell find plugin -name '*.java')
plugin_OBJECTS := $(plugin_SOURCES:.java=.class)
plugin_FILES := plugin/org/dejavu/icons.properties # todo: automate this

plugin_JFLAGS := -sourcepath plugin -classpath ../LateralGM/lgm16b4.jar

dejavu.jar: $(plugin_OBJECTS) $(plugin_FILES) plugin/META-INF/MANIFEST.MF
	$(JAR) cmf plugin/META-INF/MANIFEST.MF $@ $(^:plugin/%=-C plugin %)

%.class: %.java
	$(JAVAC) $(plugin_JFLAGS) $<

# build the library

library_SOURCES := $(shell find compiler system plugin -name '*.cc')
library_OBJECTS := $(library_SOURCES:.cc=.o)
library_DEPENDS := $(library_SOURCES:.cc=.d)

library_CXXFLAGS := -fpic
library_CPPFLAGS := $(shell llvm-config --cppflags) -I$(JAVA_HOME)/include -I$(JAVA_HOME)/include/linux
library_LDFLAGS := -shared $(shell llvm-config --ldflags)
library_LDLIBS := $(shell llvm-config --libs core native scalaropts ipo linker bitwriter)

dejavu.so: $(library_OBJECTS)
	$(CXX) $(library_LDFLAGS) -o $@ $^ $(library_LDLIBS)

%.o: %.cc
	$(CXX) -c -std=c++11 -Iinclude $(DEPFLAGS) $(CXXFLAGS) $(library_CXXFLAGS) $(library_CPPFLAGS) -o $@ $<

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
-include $(library_DEPENDS) $(runtime_DEPENDS)
endif
