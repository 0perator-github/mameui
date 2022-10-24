# MAMEUI

## What is MAMEUI?

MAMEUI is a modification of the MAME project which adds a Windows-based front-end GUI to MAME.

MAMEUI essentially converts MAME to a Windows desktop application, allowing it to run from the Windows desktop. Back in 1997, before MAME had a GUI or a front-end, John IV worked on MAME32. It eventually became known as MAME(UI32/UI64) in the fall of 2007 after John IV decided to release both 32-bit and 64-bit versions. Years later, in 2016, it was handed over to Robbbert, who maintained the project on GitHub until November 17, 2022, when he quit. So, I decided to start my own repository on GitHub because, for years, I've been tracking and experimenting with the code from MAMEUI.

## Where can I find out more?

* [MAMEUI Site](https://messui.1emulation.com/)
* [Official MESS Wiki](http://mess.redump.net/)
* [Official MAME Development Team Site](https://www.mamedev.org/) (includes binary downloads, wiki, forums, and more)
* [MAME Testers](https://mametesters.org/) (official bug tracker for MAME)

### Community

* [MAMEUI forum](http://www.mameworld.info/ubbthreads/postlist.php?Cat=&Board=mameui)
* [MAME Forums on bannister.org](https://forums.bannister.org/ubbthreads.php?ubb=cfrm&c=5)
* [r/MAME](https://www.reddit.com/r/MAME/) on Reddit

## Development

![Alt](https://repobeats.axiom.co/api/embed/9d9e119e90587ddd503aac47b5d216c943e20fe9.svg "Repobeats analytics image")

### CI status and code scanning

[![CI (Linux)](https://github.com/0perator-github/mameui/workflows/CI%20(Linux)/badge.svg)](https://github.com/0perator-github/mameui/actions/workflows/ci-linux.yml) [![CI (Windows](https://github.com/0perator-github/mameui/workflows/CI%20(Windows)/badge.svg)](https://github.com/0perator-github/mameui/actions/workflows/ci-windows.yml) [![CI (macOS)](https://github.com/0perator-github/mameui/workflows/CI%20(macOS)/badge.svg)](https://github.com/0perator-github/mameui/actions/workflows/ci-macos.yml) [![Compile UI translations](https://github.com/0perator-github/mameui/workflows/Compile%20UI%20translations/badge.svg)](https://github.com/0perator-github/mameui/actions/workflows/language.yml) [![Build documentation](https://github.com/0perator-github/mameui/workflows/Build%20documentation/badge.svg)](https://github.com/0perator-github/mameui/actions/workflows/docs.yml) [![Coverity Scan Status](https://scan.coverity.com/projects/31315/badge.svg?flat=1)](https://scan.coverity.com/projects/0perator-github-mameui)

### How to compile?

You can only build MAMEUI on a Windows computer. It won't compile on Unix.

```
make STRIP_SYMBOLS=1
```

for a full build,


```
make SUBTARGET=tiny STRIP_SYMBOLS=1
```

for a build including a small subset of supported systems.

In order to build MAMEUI using Clang use the following command:

```
make ARCOPTS='-fuse-ld=lld' OVERRIDE_AR='llvm-ar' OVERRIDE_CC='clang' OVERRIDE_CXX='clang++' STRIP_SYMBOLS=1
```

Visual Studio builds are also possible, but you'll still need a [build environment](http://www.mamedev.org/tools/) based on MinGW-w64.
In order to generate a solution and its accompanying project files, just run:

```
make vs2022
```
or use this command to build it directly using msbuild:

```
make vs2022 MSBUILD=1
```

### Coding standard

Information can be found at https://github.com/mamedev/mame#coding-standard

## Licensing Information

Information about the MAME license can be found at https://github.com/mamedev/mame#license

Information about the WINUI portion can be found in [winui_license.txt](https://raw.githubusercontent.com/0perator-github/mameui/refs/heads/master/docs/winui_license.txt)
