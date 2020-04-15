# Hardware Configuration

Before building and flashing the firmware you must review and possibly adjust the hardware configuration.

The default configuration is for using the "JHEMCU Play F4" (or sometimes named "JMT Play F4") whoop
sized flight controller coupled to an NRF24L01 transceiver module. RPM filtering is enabled (which
requires flashing the onboard ESCs with bidirectional DSHOT support using either JESC or JazzMaverick.
The RPM filtering configuration also expects your motors to have 12 poles (as I use 0802
motors). Most small motors for whoop or micros (08XX, 11XX, 12XX) tend to have 12 poles.

The above configuration is what I typically use but by adjusting various configuration files you should be
able to customize SilverLite to match your needs.

Only two [flight controller targets](Targets.md) are currently supported (NOX and OMNIBUSF4), while
[4 different transceiver modules](Transceiver.md) can be used. IBUS support is currently a work in progress
(my current plan is to use the [IBUS code](https://github.com/NotFastEnuf/NFE_Silverware/blob/master/Silverware/src/rx_ibus.c) found in NFE Silverware as a starting point; the comments in that code indicate that it was originally contributed by BobNova.

The Play F4 board is a `NOX` flight controller target. There are enough pads on this FC board to support a few different ways to wire it up to
an NRF24L01 (or XN297 etc). I've tried at least two different wiring configurations as well as a few different
locations and orientations to mount the board. If you want to do something different than the default
there should be enough information in this document to help you figure it out. If not, please open an issue on my github project page so I can help.

At a minimum the following two files should be carefully reviewed and edited to meet your custom needs:

* `_myConfig.h`
    * Defines for: rates (acro and level mode), expo curves, PID terms, looptime, RPM filter enable/disable, low pass filters, TX switches/channels, gyro (board) orientation, motor order, sticks deadband
* `_myHardware.h`
    * Defines for: DSHOT configuration, idle offest, motor pole count

The choice of RX implementation (NRF24L01, XN297, LT8900, etc) may also require you to review and edit
these files:

* `trx.h`
    * Define exactly one of the following (`TRX_NRF`, `TRX_XN297`, `TRX_XN297L`, `TRX_LT8900`) to specify which transceiver board you'll be using. Make sure the others are commented out.
* `trx_spi_config.h`
    * Used to define which STM32 pins to use for software SPI implementation. In other words what pads on your flight controller board
    will be wired up to the pads on the the transceiver module board.

# Advanced configuration
If you want to remap the motors or change how your flight controller board is oriented (flipped, or rotated 45 degrees, etc),
or wish to use a different configuration then the default then this section will hopefully contain the information
you need.

## Rates and PID terms
