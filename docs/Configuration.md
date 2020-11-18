# Hardware Configuration

Before building and flashing the firmware you must review and possibly adjust the hardware configuration.

Here is a short checklist of things to consider:

* What flight controller board will you be using? [Several boards are supported.](Targets.md)
* What type of receiver will you be using?
    * On-board SPI AFHDS/AFHDS2A is available when using the "HappyModel Crazybee F4 Lite 1S" flight controller (found on the Mobula6). This is a `MATEKF411RX` target board.
    * External SPI Bayang is available using various [transceiver modules.](Transceiver.md).
        * I've had great success with the [Play F4 board and an NRF24L01](PlayF4_NRF24L01.md).
* Configure your aux channels: throttle kill switch (similar to an arm switch), level mode enable/disable, motor beeps, etc
* Will you be using RPM filtering?
    * If so, you'll want to make sure your ESCs are flashed with either [JESC](https://jflight.net/) or [JazzMaverick](https://github.com/JazzMaverick/BLHeli/tree/JazzMaverick-patch-1/BLHeli_S%20SiLabs).

## Specifying the flight controller

Only three [flight controller targets](Targets.md) are currently supported (`MATEKF411RX`, `NOX`, `OMNIBUSF4`). The `MATEKF411RX` target can support an on-board SPI transceiver (only FlySky AFHDS/AFHDS2A is supported at this time). My fleet of whoops and micros are currently using the "Play F4" board (`NOX`) or the "HappyModel Crazybee F4 Lite 1S" flight controller (found on the Mobula6). This is a `MATEKF411RX` target.

You will not need to edit any source files to specify your target board. Instead you just need to make note of the target name
(such as `NOX` or `MATEKF411RX`). You'll need to know this when you build the firmware as described in the [Develop](Develop.md) section of this document.

## Specifying the receiver

If you wish to use an external SPI RX transceiver [4 different modules](Transceiver.md) are supported. IBUS support is also available; I've successfully used the FlySky FS-RX2A receiver with IBUS on the `NOX` and `OMNIBUSF4` targets. And if you're using that Mobula6 board, then you can use the on-board SPI AFHDS/AFHDS2A receiver.

You will edit the `_my_config.h` source file to specify your receiver option. Using a text editor (I suggest Visual Studio Code) open the file and look near the top of the file for something that looks like this:

```c++
//------------------------------------------------------------------------------
// RX protocol and configuration
// Enable only one of the following defines
//------------------------------------------------------------------------------
//#define RX_SILVERLITE_BAYANG_PROTOCOL   // Enable SilverLite SPI Transceiver RX implementation
//#define RX_IBUS // Enable IBUS protocol support on a USART RX pin, double-check rx_ibus.cpp and define one of: FLYSKY_i6_MAPPING, TURNIGY_EVOLUTION_MAPPING
#define RX_FLYSKY   // Enable FlySky SPI transceiver implementation
```

The lines that start with `//` are "commented out", meaning they don't do anything.
You basically want to comment out 2 of the 3 choices. The example above enables
the FlySky (AFHDS/AFHDS2A) SPI transceiver option. If instead you want to use an external RX module
with IBUS protocol then you'd edit that text to look like this intead:

```c++
//------------------------------------------------------------------------------
// RX protocol and configuration
// Enable only one of the following defines
//------------------------------------------------------------------------------
//#define RX_SILVERLITE_BAYANG_PROTOCOL   // Enable SilverLite SPI Transceiver RX implementation
#define RX_IBUS // Enable IBUS protocol support on a USART RX pin, double-check rx_ibus.cpp and define one of: FLYSKY_i6_MAPPING, TURNIGY_EVOLUTION_MAPPING
//#define RX_FLYSKY   // Enable FlySky SPI transceiver implementation
```

If you choose the `RX_SILVERLITE_BAYANG_PROTOCOL` option (an external SPI transceiver module
configured to run Bayang protocol), then it would look like the snippet below, *plus* you'll
also want to review and possibly edit the text immediately below that.

```c++
//------------------------------------------------------------------------------
// RX protocol and configuration
// Enable only one of the following defines
//------------------------------------------------------------------------------
#define RX_SILVERLITE_BAYANG_PROTOCOL   // Enable SilverLite SPI Transceiver RX implementation
//#define RX_IBUS // Enable IBUS protocol support on a USART RX pin, double-check rx_ibus.cpp and define one of: FLYSKY_i6_MAPPING, TURNIGY_EVOLUTION_MAPPING
//#define RX_FLYSKY   // Enable FlySky SPI transceiver implementation

//------------------------------------------------------------------------------
// When using RX_SILVERLITE_BAYANG_PROTOCOL you must specify which transceiver
// module you're using and whether or not you're using 3-wire SPI or 4-wire SPI.
//
// Note:  The software SPI pins used for interfacing with the module are defined 
// in file: trx_spi_config.h
//------------------------------------------------------------------------------
#ifdef RX_SILVERLITE_BAYANG_PROTOCOL

// Define only one of the TRX_??? values below
#define TRX_NRF
//#define TRX_XN297
//#define TRX_XN297L
//#define TRX_LT8900
```

See the line that says `// Define only one of the TRX_??? values below`?
You'll do something similar here, you'll comment out 3 of the 4 choices.
The snippet above has enabled only the `TRX_NRF` option. This means you'll be
using an NRF24L01 transceiver module for your receiver (and also to transmit back
telemetry).

### Configuring SPI

When using an external SPI transceiver module you will want to review the pin assignments that
connect the STM32 processor to your external module. 

Examine the `trx_spi_config.h` header file and it should be commented well enough to figure out how
things are hooked up and how you can change things if needed.

The [Play F4 and NRF24L01 section](PlayF4_NRF24L01.md) provides a pretty good walkthru of how to
connect an NRF24L01 module to a "Play F4" flight controller board.


## Configuring Rates and Expo
The next section of the `_my_config.h` file is where you'll define your rates and expo.

```c++
//------------------------------------------------------------------------------
// Rates
//------------------------------------------------------------------------------

// rate in deg/sec for acro mode
#define MAX_RATE            800
#define MAX_RATEYAW         675

#define LEVEL_MAX_ANGLE     80
#define LEVEL_MAX_RATE      900

#define LOW_RATES_MULTI     0.65

//------------------------------------------------------------------------------
// Expo
//  Allowed values are 0.00 to 1.00. A value of 0 means no expo applied
//  The higher the value, the less sensitive near center
//------------------------------------------------------------------------------

#define ACRO_EXPO_ROLL      0.85
#define ACRO_EXPO_PITCH     0.85
#define ACRO_EXPO_YAW       0.26

#define ANGLE_EXPO_ROLL     0.55
#define ANGLE_EXPO_PITCH    0.55
#define ANGLE_EXPO_YAW      0.30

```

## Configuring PID terms

The next section in `_my_config.h` is where you'll define your PID terms for acro and angle (level) modes.

```c++
//------------------------------------------------------------------------------
// PID term overrides
//------------------------------------------------------------------------------
                        //  Roll    Pitch   Yaw
#define     ACRO_P      {   .040,   .040,   .01     };
#define     ACRO_I      {   .250,   .250,   .50     };
#define     ACRO_D      {   .035,   .035,   .0      };

// Angle mode P and D terms
#define     ANGLE_P1    10.
#define     ANGLE_D1    3.0
```

The values shown here are decent starting points for 65mm or 75mm whoops as well as 2.5" and 3" micros using 0802, 1102 or 1103 motors.

You can always fine tune them in the field using stick gestures and the on screen display.

*TODO*: Document the PID tuning feature


## More and more
The rest of the `_my_config.h` file is where you configure even more options that 
are available to you. I won't go into great detail on them but here's a list of what to expect:

* `RPM_FILTER` - If defined, this will enable RPM filtering. By default this is defined for all targets. Be sure you've already flashed your ESCs with firmware that supports RPM filtering.
* `LOOPTIME` - By default the looptime is configured for 250us (4k loop). You really can't go higher unless you disable RPM filtering and/or overclock. This value works very well with the targets supported by SilverLite.
* Various filters. Read the comments in the source file for more info
* Switches/Channels - What aux channels (switches on your transmitter) are supported and what they should do. This section is used to map 4 features (`THROTTLE_KILL_SWITCH`, `LEVELMODE`, `MOTOR_BEEPS_CHANNEL` and `RATES`) to some Bayang channels. It requires you know what Bayang channels your TX is setup to use when it transmits with the Bayang protocol.
    * Note: If you're using IBUS or AFHDS/AFHDS2 protocol you will instead need to edit some code in `rx_flysky.cpp` or `rx_ibus.cpp`. I apologize in advance for not documenting this further. 

### Even more config

If you have need to tweak the DSHOT configuration, or motor idle offset or motor pole count then you'll want to review and edit the `_myHardware.h` file.

