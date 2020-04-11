

# Processor and Flight Controller Targets

This codebase currently supports the STM32F411 and STM32405 processors. The `Makefile` 
will define `STM32F405xx` or `STM32F411xE` as appropriate based on the *Flight Controller Target*.

There are a few places in the code that are conditionally compiled based on those definitions.

The following flight controller targets are currently defined:

* `OMNIBUSF4` 	- This uses the STM32F405 processor which runs at 168Mhz
* `NOX`         - This uses the STM32F411 processor which runs at 98Mhz or 100Mhz (we must use 98Mhz for proper USB Virtual Com Port support)

The `Makefile` defaults to building for the `NOX` target but this can be overriden when invoking
the makefile by specifying the `TARGET` on the command line like so:

```
mingw32-make.exe -j12 flash TARGET=OMNIBUSF4
```

> Note: I'm still in the early stages of adding support for the F3 processor and will update this
document when it becomes available. The first F3 based flight controller target I plan to support
will be the OMNIBUS board and then the CRAZYBEEF3 (FlySky) board.

