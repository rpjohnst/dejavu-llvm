---
layout: default
title: Home
---

## DejaVu
DejaVu is a free, open source implementation of [GameMaker](http://yoyogames.com). It allows you to design and build quality games quickly and without code. You can do anything with the games you create, for free. DejaVu exports games to Windows, OS X, Linux, Android, and iOS.

### Current Status
DejaVu is currently a work in progress. You can't use it yet, but the source is available [on GitHub](http://github.com/rpjohnst/dejavu).

What DejaVu is currently capable of:
* Getting game data from [LateralGM](http://lateralgm.org) through a plugin
* Compiling GML and D&D actions in scripts and object events using LLVM
* Linking with a barebones GML runtime library

What's next:
* Finalizing the calling convention for script arguments
* Exporting executables using a system linker (ideally LLVM's LLD, but until it supports more executable formats it will be the system linker)
* Writing game assets into the executable
* The GML standard library and game engine
* New IDE with better D&D support and native widgets
