# TODO

This is where I'll document how to install a transceiver board onto your flight controllers
as well as how what adjustments you may need to perform to various configuration files (mostly header files).

## Using onboard SPI

Currently only the FlySky version of the "HappyModel Crazybee F4 Lite 1S" flight controller (found on the Mobula6) us supported.
Since this is built into the flight controller board the only thing you need to configure is whether you want
to use the older AFHDS protocol (which has a minimal latency of 1.5ms) or the newer AFHDS2A protocol (minimal latency of 3.850ms).


## For external SPI RX modules

Things to discuss:

* Describe the available choices of transceiver modules
    * NRF24L01 
        * Comes in different form factors
            * The micro version like [this one on Amazon](https://www.amazon.com/SMAKN%C2%AE-NRF24L01-Wireless-Transceiver-Arduino/dp/B0181PDTNU) works well.
        * Many ebay, Aliexpress listings say they are NRF24L01 but are something completely different (not even a clone of the NRF24L01), so beware!
        * With Play F4 board you must use capacitor across +3.3v and GND due to the noisy power supply. In comparison the XN297L seemed okay without a cap on the Play F4.
    * XN297L
        * Uses 3-wire SPI
        * Easily harvested from H8mini transmitter
    * LT8900
        * Requires an extra pin or a resister/capacitor combo to perform a reset on power-on.
        * Requires a customized multiprotocol module firmware. I built a custom TX variant for this but never pushed this up to github.
* Need to edit `_my_config.h` to specify which transceiver module you're using. Just need to define only one of the following choices:
    * `TRX_NRF`
    * `TRX_XN297`
    * `TRX_XN297L`
    * `TRX_LT8900`
* Need to edit `_my_config.h` to specify if you'll be using 3-wire SPI or 4-wire SPI
    * Enable the `TRX_SPI_3WIRE` for 3-wire, otherwise it will be 4-wire
* Need to edit `trx_spi_config.h` to specify which STM32 pins to use for the software SPI.
    * The [Play F4 and NRF24L01](PlayF4_NRF24L01.md) section has more detail on this.
* Wiring considerations
    * Avoid inductor coil
* Antenna mods

