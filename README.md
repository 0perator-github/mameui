 
# **MAMEUI** #

Continuous integration build status:

| OS/Compiler                 | Status        |
| --------------------------- |:-------------:|
| Linux/clang and GCC         | ![CI (Linux)](https://github.com/0perator-github/mameui/workflows/CI%20(Linux)/badge.svg) |
| Windows/MinGW GCC and clang | ![CI (Windows)](https://github.com/0perator-github/mameui/workflows/CI%20(Windows)/badge.svg) |
| macOS/clang                 | ![CI (macOS)](https://github.com/0perator-github/mameui/workflows/CI%20(macOS)/badge.svg) |
| UI Translations             | ![Compile UI translations](https://github.com/0perator-github/mameui/workflows/Compile%20UI%20translations/badge.svg) |
| Documentation               | ![Build documentation](https://github.com/0perator-github/mameui/workflows/Build%20documentation/badge.svg) |
| BGFX Shaders                | ![Rebuild BGFX shaders](https://github.com/0perator-github/mameui/workflows/Rebuild%20BGFX%20shaders/badge.svg) |

What is MAMEUI?
===============
MAMEUI is a modification of the MAME project that converts MAME to a windows desktop application, which acts as a built in front-end. It allows you to select and run vintage software from the windows desktop.

Windows 7 or later is required.

How to compile?
===============

You can only build MAMEUI on a Windows computer. It won't compile on Unix.

```
make STRIP_SYMBOLS=1
```

for a full build,

```
make SUBTARGET=tiny STRIP_SYMBOLS=1
```

for a build including a small subset of supported systems.

You can also build MAMEUI using Clang in the MSYS environment.

```
make ARCOPTS='-fuse-ld=lld' OPTIMIZE=1 OVERRIDE_AR='llvm-ar' OVERRIDE_CC='clang' OVERRIDE_CXX='clang++' STRIPSYMBOLS=1
```

See the [Compiling MAME](http://docs.mamedev.org/initialsetup/compilingmame.html) page on our documentation site for more information, including prerequisites for macOS and popular Linux distributions.

For recent versions of macOS you need to install [Xcode](https://developer.apple.com/xcode/) including command-line tools and [SDL 2.0](https://github.com/libsdl-org/SDL/releases/latest).

For Windows users, we provide a ready-made [build environment](http://www.mamedev.org/tools/) based on MinGW-w64.

Visual Studio builds are also possible, but you still need [build environment](http://www.mamedev.org/tools/) based on MinGW-w64.
In order to generate solution and project files just run:

```
make vs2019
```
or use this command to build it directly using msbuild

```
make vs2019 MSBUILD=1
```


Where can I find out more?
==========================

* [Official MAME Development Team Site](https://www.mamedev.org/) (includes binary downloads, wiki, forums, and more)
* [Official MESS Wiki](http://mess.redump.net/)
* [MAMEUI site] http://www.mameui.info/
* [MAMEUI forum] http://www.mameworld.info/ubbthreads/postlist.php?Cat=&Board=mameui


Licensing Information
=====================

Information about the MAME content can be found at https://github.com/mamedev/mame/blob/master/README.md

Information about the MAME license can be found at https://github.com/mamedev/mame/blob/master/COPYING

Information about the WINUI portion can be found at https://github.com/0perator-github/mameui/blob/master/docs/winui_license.txt

