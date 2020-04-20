# Hardware Configuration

Before building and flashing the firmware you must review and possibly adjust the hardware configuration.

The default configuration is for using the "JHEMCU Play F4" (or sometimes named "JMT Play F4") whoop
sized flight controller coupled to an NRF24L01 transceiver module. RPM filtering is enabled (which
requires flashing the onboard ESCs with bidirectional DSHOT support using either [JESC](https://jflight.net/) or [JazzMaverick](https://github.com/JazzMaverick/BLHeli/tree/JazzMaverick-patch-1/BLHeli_S%20SiLabs).
The RPM filtering configuration also expects your motors to have 12 poles (as I use 0802
motors). Most small motors for whoop or micros (08XX, 11XX, 12XX) tend to have 12 poles.

The above configuration is what I typically use but by adjusting various configuration files you should be
able to customize SilverLite to match your needs.

Only two [flight controller targets](Targets.md) are currently supported (`NOX` and `OMNIBUSF4`), while
[4 different transceiver modules](Transceiver.md) can be used. 

> Note: IBUS support is currently a work in progress and a proof of concept has been bench tested. I'll update this
document further once I've verified it is usable.

The Play F4 board is a `NOX` flight controller target. There are enough pads on this FC board to support a few different ways to wire it up to
an NRF24L01 (or XN297 etc). I've tried at least two different wiring configurations as well as a few different
locations and orientations to mount the board. If you want to do something different than the default
there should be enough information in this document to help you figure it out. If not, please open an issue on my github project page so I can help.

At a minimum the following two files should be carefully reviewed and edited to meet your custom needs:

* `_myConfig.h`
    * Defines for: rates (acro and level mode), expo curves, PID terms, looptime, RPM filter enable/disable, low pass filters, TX switches/channels, gyro (board) orientation, motor order, sticks deadband
* `_myHardware.h`
    * Defines for: DSHOT configuration, idle offest, motor pole count. Default config is DSHOT300, 12 pole motors, 

I've structured and commented these header files so that it should hopefully be self explanatory when it comes to
enabling and configuring the features.

The choice of RX implementation (NRF24L01, XN297, LT8900, etc) may also require you to review and edit
these files:

* `trx.h`
    * Define exactly one of the following (`TRX_NRF`, `TRX_XN297`, `TRX_XN297L`, `TRX_LT8900`) to specify which transceiver board you'll be using. Make sure the others are commented out.
* `trx_spi_config.h`
    * Used to define which STM32 pins to use for software SPI implementation. In other words what pads on your flight controller board
    will be wired up to the pads on the the transceiver module board.

> Note: While I may have written a lower level abstraction layer of code for the XN297 and XN297L modules, I have not yet had a chance to test them. ***More importantly***, the SPI code has not yet been conditionalized to support 3-wire SPI. It's actually pretty easy to complete but I just haven't had the opportunity to do this yet.

# Advanced configuration

This is just a placeholder for now. I hope to provide more details on how to configure various features. A lot of useful info can be found
on Markus's thread regarding his SilF4ware firmware.

Motor order: https://www.rcgroups.com/forums/showpost.php?p=41995581&postcount=341