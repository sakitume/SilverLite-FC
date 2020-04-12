# Building

This project uses `make` to build and optionally flash the firmware. On my Windows 10 machine
I installed [MinGW-W64](https://sourceforge.net/projects/mingw-w64/) which installs
`mingw32-make.exe`; this is the `make` tool that I use on Windows.

I typically use a "Terminal" window within Visual Studio. You can set that up to use the 
Windows CMD.exe program but I prefer a `bash` command prompt. On my Windows PC I have git for
windows installed which also installs a `bash` shell. Both work fine. I enter the following
command in that terminal to build.

```
mingw32-make.exe -j12
```

Because I did not specify a target with this command line, the firmware will be built
for the (default) NOX flight controller target. The `-j12` instructs the `make` tool 
(`mingw32-make.exe`) to use twelve jobs (threads) when compiling which speeds the build process
up substantially.

You can build the OMNIBUSF4 target by specifying it like so:

```
mingw32-make.exe -j12 TARGET=OMNIBUSF4
```

If you're developing code and make changes and would like to clean up all intermediate
object files (so you end up with a full rebuild) you can use the `clean` command like so:

```
mingw32-make.exe clean
```

> Note: When using `clean` make sure you specify the target if it isn't the default `NOX` target.


# Flashing

To flash the firmware onto a flight controller you will need to have it connected via USB
or via an ST-Link adapter.

The OMNIBUSF4 flight controller board (that I use) has breakout pads for the SWDIO and SWCLK pins
of the STM32. This lets me use an ST-Link adapter which gives me the added ability to debug
code in addition to flashing it.

If you examine the `Makefile` and look for the `flash` target you will see that it uses `openocd`
to flash the firmware when your build target is `OMNIBUSF4`. If your build target is `NOX`, the
makefile will instead use the command line version of [STM32CubeProgrammer](https://www.st.com/en/development-tools/stm32cubeprog.html)
to flash the firmware.

The NOX flight controller boards that I've been using (the "JMT Play F4" also known as "JHEMCU Play F4")
does not have the SWDIO and SWCLK pins exposed. This means I must reset the board into DFU (bootloader) mode
and connect it to my PC via USB. There are 2 boot pads on the top side near the front corner (look for "BOOT"
as shown in this pic):

![Play F4 Top](images/Play-F4-Top.jpg)

Short those two boot pads together while powering up the board (either by connecting it to your PC
via USB or by connecting a battery to the board). I find this to be rather tricky to do so I've
added a feature to the firmware that lets you easily enter bootloader mode with a simple utility
program. So after the first successful flashing of the firmware you won't have to use those
boot pads again. More on this in the "Monitor" section of this document. 

Another method you can use to enter DFU mode is to use the "L-R-D" gesture (Left/Right/Down). But
this (of course) means you need to have successfully flashed the firmware once already and are able
to connect your TX to the flight controller.

> Note: If you have an OMNIBUSF4 target and don't wish to use an ST-Link adapter then edit the `Makefile`
so that it uses the [STM32CubeProgrammer](https://www.st.com/en/development-tools/stm32cubeprog.html)
to flash the firmware. My OMNIBUSF4 flight controller board actually has a handy pushbutton to enter
DFU mode.

# Monitor

If you've successfully flashed the firmware you can use a simple utility program written in Python
to monitor the debug output of the firmware. Connect your board via USB to your PC. Determine the
com port and invoke the Python script like so (from a `bash` shell):

```
./monitor.py com11
```

Change the `com11` to whatever com port your flight controller shows up as. You can use the Windows 10
"Device Manager" tool and examine the "Ports (COM &LPT)" section. The board will show up as
"STMicroelectronics Virtual COM Port (COM11)" (the last part will likely differ for you).

I usually open another "terminal" pane in Visual Studio for this. The output will look something like this:

```
New Serial

looptime:  81
osdTime:  43
pps:  0 hit:  0
hcrc:  0 bcrc:  0

looptime:  88
osdTime:  56
pps:  0 hit:  0
hcrc:  0 bcrc:  0

looptime:  83
osdTime:  43
pps:  0 hit:  0
hcrc:  0 bcrc:  0
```

The "looptime" stuff repeats like every second or so. The firmware has a few lines of code that emit log statements
that this `monitor.py` script can echo to your screen. These values were helpful to me when I was developing the
firmware. These will probably be removed but for now I'll document them here:

* "looptime" This is the time (in microseconds) for the longest loop within the past second. The firmware runs 4000 loop iterations per second.
* "osdtime" This is how long it took (in microseconds) to update the OSD. As you can see it is pretty expensive (I'm using a software SPI implementation)
* "pps" The number of packets *successfully* received and accepted per second. 
* "hit" The number of packets received but not necessarily accepted per second
* "hcrc" The number of packet CRC failures (as detected by hardware)
* "bcrc" The number of bayang packet Checksum failures (as detected by software)

> Note: If hardware CRC checking is available then Bayang checksum failures will never occur (as the hardware CRC check will reject the packet before
the Bayang checksum would occur)

Use `Ctrl-C` in the terminal window to kill the monitor program. If you disconnect the flight controller the monitor program will continue to run
and keep trying to re-connect to the flight controller. Plugging the controller back in will usually establish a reconnect with the monitor tool.

> Note: This `monitor.py` script will only work under Windows. This is due to how I check for keypresses (particularly the `r` key). I'm sure it
could be adjusted to work on Mac OS as well as Linux. I just haven't taken the time to do so since my firmware development is performed on my
desktop PC rather than my laptops.

## Using the `monitor` to enter DFU (bootloader) mode
While the monitor tool is connected and running you can press your `r` key on your keyboard to reset the board into DFU mode. The terminal should
display the following when you do this:

```
Sending reset command
```

You will also see in the windows Device Manager tool that the Virtual COM port has disappeared and a new "STM32 BOOTLOADER" device appears
under the "Universal Serial Bus devices" section.

You can now flash new firmware to your board. Either with the make tool and `flash` target in our `Makefile`, or you can use the STM32CubeProgrammer
tool. 

