# These are notes on FlySky AFHDS and AFHDS2A protocol

Pascal's DIY Multiprotocol module has an RX implementation. See: `Multiprotocol\AFHDS2A_Rx_a7105.ino`
Betaflight rx code is here: `src\main\rx\a7105_flysky.c`
A7105-receiver/uFlySky, based off of Betaflight/Cleanflight code: `https://github.com/nikdavis/A7105-receiver/tree/master/uFlSky`

Betaflight uses an interrupt pin from the A7105 module, while MPM doesn't seem to and instead
reads A7105_00_MODE register

uFlySky seems to be a better starting point than Betaflight as it has stripped out many of the Betaflight
baggage.


