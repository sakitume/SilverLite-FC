# About

[SilverLite flight controller firmware](https://github.com/sakitume/SilverLite-FC) is a derivative of [Silverware](http://sirdomsen.diskstation.me/dokuwiki/doku.php). 
Rather than forking the [BWhoop Silverware version](https://github.com/silver13/BoldClash-BWHOOP-B-03) (from Silver13/SilverXXX)
or [NFE Silverware](https://github.com/NotFastEnuf/NFE_Silverware) (from Travis Shrock) I instead chose to fork [SilF4ware](https://www.rcgroups.com/forums/showthread.php?3294959-SilF4ware-an-STM32F4-port-of-SilverWare) (an STM32F4 port of Silverware from Markus Gritsch) and build up a set of features and extensions on top of that. If you're not already familiar with it, SilF4ware is an amazing port of Silverware with some rather impressive features! I'm awed by this video shared by Markus having ["3D Fun with SilF4ware"](https://www.youtube.com/watch?v=eGqPaot6K80). I had not realized quadcopter flight like this was possible, or was even being performed. Prepare to be amazed when you click on that link.

> Note: SilverLite flight controller is my own custom spin of Silverware that is **heavily** based on the very hard work of some extremely talented people. Among them are Markus Gritsch [(SilF4ware)](https://www.rcgroups.com/forums/showthread.php?3294959-SilF4ware-an-STM32F4-port-of-SilverWare), Travis Schrock [(NFE Silverware)](https://community.micro-motor-warehouse.com/t/notfastenuf-e011-bwhoop-silverware-fork/5501), Silver13/SilverXXX [(Silverware)](http://sirdomsen.diskstation.me/dokuwiki/doku.php) and of course so many others. My understanding of the history of Silverware is still limited and I'm sure there are numerous other contributors that should be named here as well. Kudos to all of you and thank you again for sharing and contributing so much to this wonderful hobby.

SilverLite flight controller firmware was developed for *my particular use case*: controlling 65mm or 75mm whoops and 2.5" and 3" micro quadcopters
as well as for the sheer fun of experimenting and hacking. I don't intend it to be much more than that. NotFastEnuf has been
developing (the hopefully soon to be released) [QuickSilver firmware](https://community.micro-motor-warehouse.com/t/notfastenuf-e011-bwhoop-silverware-fork/5501/1223) (the next iteration of his [NFE Silverware](https://community.micro-motor-warehouse.com/t/notfastenuf-e011-bwhoop-silverware-fork/5501)); I suspect it will
have some really cool features when it comes out. And of course Markus' [SilF4ware](https://www.rcgroups.com/forums/showthread.php?3294959-SilF4ware-an-STM32F4-port-of-SilverWare) is an amazing Silverware derivative that has
RPM filtering, blackbox logging, enhanced DSHOT implementations, etc. Please check out those other projects.

SilverLite features include:

* A minimal OSD
* IBUS support
* Bayang protocol using an external transceiver (NRF24L01, XN297, XN297L). Low latency, theoretical 2ms (or 3ms depending on transmitter), or 5ms if using telemetry.
* SPI AFHDS and AFHDS2A support (only when building for `MATEKF411RX` target).
* Enhanced telemetry and configuration if using my [SilverLite Firmware for the FlySky FS-i6](https://github.com/sakitume/SilverLite-FS-i6) or
my SilverLite TX firmware for STM32, ESP32, ESP8266 (unreleased).

# Quick Start

The navigation bar on the left provides a list of topics you'll likely be interested in. It is suggested you read these sections:

* [Configuration](Configuration.md)
    * This describes what files to edit and what options/features are available to customize.
* [Targets](Targets.md)
    * This describes what flight controller targets are supported as well as providing details on flight controller boards known to work with SilverLite.
* [Transceiver modules](Transceiver.md)
    * This describes the available transceiver modules that can be used with SilverLite (XN297, XN297L, LT8900 and NRF24L01)
* [Build and Flash](Develop.md)
    * This describes how to build the firmware and also how to flash it onto your flight controller board.
* [Development Tools Setup](DevToolsSetup.md)
    * This describes the software needed for building and flashing SilverLite with a Mac, Windows or Linux PC
