# SilverLite TX packets

Bind packets are just like normal Bayang bind packets with a few changes.
Here is a description of the normal Bayang bind packet:

* packet[0] - Identifies the bayang subprotocol
   	* 0xA4 = Stock Bayang or Silverware with no telemetry
   	* 0xA3 = Silverware with telemetry
   	* 0xA1 = Silverware with telemetry, with 2 aux analog channels
   	* 0xA2 = Silverware no telemetry, with 2 aux analog channels
* packet[1..5] - The 5 byte address of the TX
* packet[6..9] - 4 channel values to cycle through when hopping
* packet[10]	- First byte of TX address
* packet[11]	- Second byte of TX address
* packet[12]	- Third byte of TX address
* packet[13]	- 0x0A
* packet[14]	- The checksum of the previous 14 bytes

A SilverLite capable transmitter will follow the above format
with the exception of the following:

* packet[10]	- First byte of TX address XOR 0xAA
* packet[11]	- Second byte of TX address XOR 0xAA

A SilverLite capable receiver will check if these two bytes 
have this XOR'd value and set a flag if found. This lets the
flight controller send back SilverLite telemetry packets
and accept custom SilverLite TX packets.

"Normal" Bayang TX packets containing channel information follow this format:

* packet[0] - 0xA5 to indicate this is a "normal" packet
* packet[2]
	* For true Bayang:
		* 0xF4 - Use slow rates
		* 0xF7 - Use normal rates
		* 0xFA - Use fast rates
	* For Silverware with aux analog extension
		* aux channel 0. A byte value between 0..255
* packet[2] - Bit flags
    * BAYANG_FLAG_RTH      = 0x01
    * BAYANG_FLAG_HEADLESS = 0x02
    * BAYANG_FLAG_FLIP     = 0x08
    * BAYANG_FLAG_VIDEO    = 0x10
    * BAYANG_FLAG_SNAPSHOT = 0x20
* packet[3] - Bit flags
    * BAYANG_FLAG_EMG_STOP = 0x04
    * BAYANG_FLAG_INVERT   = 0x80
    * BAYANG_FLAG_TAKE_OFF = 0x20
* packet[4..5]	- A channel
* packet[6..7]	- E channel
* packet[8..9]	- T channel
* packet[10..11]- R channel
* packet[12]	- Third byte of TX address (`rx_tx_addr[2]`)
* packet[13]
	* For true Bayang/Silveware:
		* 0x0A
	* For Silverware with aux analog extension
		* aux channel 1. A byte value between 0..255
* packet[14]	- The checksum of the previous 14 bytes

The AETR channel values are each encoded into two bytes as a channel value.
Or you could also think of it as a 16bit unsigned integer stored in big-endian
format (most signifcant byte first). This unsigned integer value will be in the 
range 0..1023 requiring only 10 bits. The remaining 5 bits will either be all set
or may have some weird value I don't yet understand....something to do with "dyntrim"
(see Goebish nrf24l01 multiprotocol module or Pascal Langer's multiprotocol module for
more information).

A SilverLite TX (if configured) will override one byte of the above packet:
* packet[12]	- Third byte of TX address (`rx_tx_addr[2]`) XOR 0xAA

This is used as a signal to the RX that the transmitter is SilverLite capable. This is
necessary in case the RX skipped the bind phase completely due to it being previously
configured for auto-bind upon startup.

If the SilverLite TX has received confirmation from the RX that it too is SilverLite
capable (via a special telemetry packet), then the "normal" packet can be further
modified.
* packet[4]		- Upper 5 bits only. A command code between 0..31 inclusive
* packet[6]		- Upper 5 bits only
* packet[8]		- Upper 5 bits only
* packet[2]		- Entire byte
* packet[12]	- Entire byte
* packet[13]	- Entire byte

The command code in `packet[4]` is used to determine how the other bits and bytes
should be used.

# A better PID tweaking implementation

With the standard Silverware `aux_analog[]` extension, if we want to be able to 
adjust the I term using an aux_analog[] channel we'd need a third channel which we don't have. 
If we wanted to adjust Yaw PID terms we'd need even more channels.
Also, this extension uses these `aux_analog[]` values to scale some computations
that use the P and D terms; it is not actually modifying the P and D terms themselves.

The SilverLite extension uses only a single aux analog channel in conjunction with different command code
to modify the PID terms directly. The command mode is sent with each packet as part of
the SilverLite extension.

* kTweakPD_RollPitch
	* `aux_analog[0]` is used to update `pidkp[]` and `pidkd[]` for roll and pitch
* kTweakI_RollPitch 			
	* `aux_analog[0]` is used to update `pidki[]` for roll and pitch
* kTweakPD_Yaw
	* `aux_analog[0]` is used to update `pidki[]` for yaw
* kTweakI_Yaw
	* `aux_analog[0]` is used to update `pidki[]` for yaw
* kTweakDisabled
	* `aux_analog[]` is ignored and pid values are untouched
* kTweakCalibrate
	* `aux_analog[0]` value should be recorded and used as a baseline reference.	

Most importantly, the `aux_analog[0]` value is not used directly. Instead it's relative
change is used to increment or decrement the PID term(s). A baseline should have been
set (via the `kTweakCalibrate` command). This baseline is subtracted from the
`aux_analog[0]` value, scaled (usually by 1.0f) and the final result is added to
the PID term being modified.

> Note: The `kTweakCalibrate` command is sent a few times in between every mode change
so that the new mode will work correctly.


