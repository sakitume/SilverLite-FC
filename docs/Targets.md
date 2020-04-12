

# Processor and Flight Controller Targets

This codebase currently supports the STM32F411 and STM32405 processors. The `Makefile` 
will define `STM32F405xx` or `STM32F411xE` as appropriate based on the *Flight Controller Target*.

There are a few places in the code that are conditionally compiled based on those definitions.

The following flight controller targets are currently defined:

* `OMNIBUSF4` 	- This uses the STM32F405 processor which runs at 168Mhz
* `NOX`         - This uses the STM32F411 processor which runs at 98Mhz or 100Mhz (we must use 98Mhz for proper USB Virtual Com Port support)

> Note: A *target* is simply the name of a hardware configuration (processor and peripherals) that
was defined for use with Betaflight. Different flight controller boards can be made that
correspond to a given target. 

The `Makefile` defaults to building for the `NOX` target but this can be overriden when invoking
the makefile by specifying the `TARGET` on the command line like so:

```
mingw32-make.exe -j12 flash TARGET=OMNIBUSF4
```

The above command will build the `OMNIBUSF4` target and flash the build onto the flight controller board.
For more details on how to build and flash (and develop) this software read the [Develop](Develop.md) page

> Note: I'm still in the early stages of adding support for the F3 processor and will update this
document when it becomes available. The first F3 based flight controller target I plan to support
will be the OMNIBUS board and then the CRAZYBEEF3 (FlySky) board.


# NOX 

I've been using the "Play F4" flight controller (sometimes described as "JMT Play F4" or "JHEMC Play F4").
This board is a NOX target. This is what it looks like:

![Play F4 Top](images/Play-F4-Top.jpg)
![Play F4 Bottom](images/Play-F4-Bot.jpg)


By examining the Betaflight source code and `target.h` files for any
Betaflight target (as well as using the `resource` and `resource list` commands) we can learn
how the STM32 processor interfaces with the MPU, OSD and other peripherals.

For example, the following pads of the Play F4 board map to the STM32 pins:
 
 On top side of board are:

* 3.3v
* DSM/IBUS/PPM  - Goes directly to PB10 (verified with multimeter)
* SBUS      - Coupled thru a switchable inverter (controlled by PC14) and then (I think) to PA3 (USART2 RX)
* 5v
* GND
* TX1       - Goes directly to PB6 (USART1 TX)

On back (bottom) side of board are:

* RX1       - Goes directly to PB7 (verified with multimeter) (USART1 RX)
* TX2       - Goes directly to PA2 (verified with multimeter) (USART2 TX)
* LED_STRIP - Goes directly to PA0 (verified with multimeter)
* BZ-       - Does not seem to be directly tied to STM32, probably uses a driver transistor to PC13

This information is important if you want to change which pads to use to interface
to your transceiver board (NRF24L01, XN297, XN297L or LT8900). More information can be found
in the [Configuration](Configuration.md) section.