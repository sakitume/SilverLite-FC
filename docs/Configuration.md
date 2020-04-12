# Configuration

At a minimum the following two files should be carefully reviewed and edited to meet your custom needs:

* `_myConfig.h`
* `_myHardware.h`

The choice of RX implementation (NRF24L01, XN297, LT8900, etc) may also require you to review and edit
these files:

* `trx.h`
    * Define exactly one of the following (`TRX_NRF`, `TRX_XN297`, `TRX_XN297L`, `TRX_LT8900`) to specify which transceiver board you'll be using.
* `trx_spi_config.h`
    * Used to define which STM32 pins to use for software SPI implementation.