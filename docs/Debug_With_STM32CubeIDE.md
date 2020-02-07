
Run `openocd` like so:

```
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg
```

Under "Debug Configurations", "Main" tab:
"Project" shold be the project you're using 
For "C/C++ Application"
`obj\main\betaflight_OMNIBUSF4.elf`.

Don't use the "Browse" button and select the elf, this results in a full path being specified
which will cause a weird error. It needs to be relative to the project and use windows slash characters
Also select the "Disable auto build" option

Under "Debug Configurations", "Debugger" tab. For "GDB Command:" browse and select this file:
`C:\Tools\armcc\bin\arm-none-eabi-gdb.exe`

For "Remote Target" tick the "Use remote target" checkbox.
For "JTAG Device" Choose "ST-Link (OpenOCD)"
The host and port should already be correct: localhost, 3333

Under "Debug Configurations", "Startup" tab. 
Uncheck the "Load image" box. This just causes problems. Better to use a separate build/flash process for that.

#--------------

Using this guide: https://github.com/ethanhuanginst/STM32CubeIDE-Workshop-2019/tree/master/hands-on/02_stm32_hp141_lcd

Create a new STM32 project

Enter STM32F405RG. Then in the "Project Setup" dialog be sure to
* Name the project "Betaflight"
* Uncheck "Use default location" and then click "Browse" and navigate to where you want the project and create a new folder and select it.
* Under "Target Project Type" be sure to choose "Empty" instead of the default "STM32Cube".


Delete all of the source files (from the Prjoect Explorer). Then in Windows explorer navigate to that folder and you'll see you have these
* `.settings` folder
* `.cproject` file
* `.project` file

Copy these three into your Betaflight local repo folder. Then you can double-click the `.project` file to launch STM32CubeIDe.
Right click the "Betaflight" project in "Project Explorer" and in the popup menu choose "Properties". Under "C/C++ Build", "Builder Setttings"
be sure to uncheck the "Generate Makefiles automatically" option. For the "Builder" section the "Builder type" should be "External Builder".
And uncheck the "Use default build command" and in the "Build command" input field enter:

```
mingw32-make -j12
```

specify 
SHELL="C:\Program Files\Git\git-bash.exe"


# Using `mingw32-make`
Building Betaflight with mingw32-make. Set `ARM_SDK_DIR` to the location of your gcc arm toolchain. For me it would be `/c/Tools/armcc`.

```
mingw32-make -j12 TARGET=NOX ARM_SDK_DIR=/c/Tools/armcc
```

```
mingw32-make -j12 TARGET=NOX ARM_SDK_DIR=/c/Tools/armcc DEBUG=GDB
```



