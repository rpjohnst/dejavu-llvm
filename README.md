DejaVu Game Maker Compiler
==========================

DejaVu is an in-progress LLVM-based GML and D&D compiler, mostly compatible with [GameMaker](http://yoyogames.com/gamemaker). It currently exists as a C++ library, and as a plugin to [LateralGM](http://lateralgm.org). It is capable of compiling a game's scripts and object events. Eventually it will be able to produce fullly compiled executables.

Build Dependencies
------------------

As a work in progress, DejaVu does not currently distribute compiled binaries. To build it from source requires several tools and libraries that should be available from your package manager or from their respective websites:

* [LLVM](http://llvm.org) - the compiler uses LLVM to generate code
* [Clang](http://clang.llvm.org) - Clang is required to build the language runtime
* [GNU Make](http://www.gnu.org/software/make/) - the compiler's Makefiles are based on GNU Make
* [SWIG](http://www.swig.org) - SWIG is used to generate the bridge between the plugin and compiler
* [Java Development Kit](http://www.oracle.com/technetwork/java/javase/downloads/index.html) - the LateralGM plugin is written in Java
* [Ant](http://ant.apache.org/) - Ant is used to compile the LateralGM plugin

### Windows

Currently, DejaVu is not tested on Windows, but there is nothing inherently preventing it from being built or running on that platform. The [MinGW](http://mingw.org) toolchain should be able to support the required dependencies, although some modifications to the Makefiles and source code may be necessary. Contributions would be welcome.

Runtime Dependencies
--------------------

DejaVu itself does not currently have any runtime dependencies. However, it currently reads game data through LateralGM:

* [Java Runtime Environment](http://www.java.com)
* [LateralGM](http://lateralgm.org)

Building and Installing
-----------------------

Once prerequisites are installed, DejaVu can be built by running `make`. This should produce three files: `dejavu.jar`, `dejavu.so`, and `runtime.bc`. The first two need to be in a directory next to LateralGM named `plugins` and the third needs to be in its working directory.

Running
-------

Running LateralGM normally will load the DejaVu plugin, adding a menu and buttons that invoke DejaVu's compiler on the loaded game. The resulting file is an optimized LLVM bitcode module containing the generated code for the game's scripts and objects' events, including D&D actions.

Contact Information
-------------------

DejaVu is available on GitHub, at [github.com/rpjohnst/dejavu](https://github.com/rpjohnst/dejavu). Feel free to file bugs, submit pull requests, or fork the repository. It is available under the terms in `LICENSE` (University of Illinois/NCSA Open Source License); the runtime is available under the terms in `runtime/LICENSE` (MIT License).
