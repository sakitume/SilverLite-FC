# Setting up your development environment
This page will walk you through the steps of setting up your development environment so that you can build and deploy this project onto your flight controller board.

It should be possible to develop on Mac OS, Linux and Windows platforms. This is due to using a variety of open source tools and technologies that are supported on all of these operating systems.

> Note: The specific examples provided here will often be written from the perspective of using a Windows development PC. I hope to update this document in the future to provide more specific examples for Mac OS and Linux platforms.

# What we'll need
Here are the software tools you'll need:

* [ARM gnu toolchain](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads)
    * This is the compiler toolchain we'll use to build the firmware
* [mingw-w64](https://sourceforge.net/projects/mingw-w64/) (if you will be developing under Windows)
    * This is needed solely for the `make` tool, or more specifically `mingw32-make.exe`
* [OpenOCD](https://gnutoolchains.com/arm-eabi/openocd/)
    * This is only needed if you wish to use an ST-Link adapter to flash your flight controller.
* [STM32CubeProgrammer](https://www.st.com/en/development-tools/stm32cubeprog.html)
    * This is a simple to use flashing tool. It has a GUI interface as well as a command line interface.
* [Visual Studio Code](https://code.visualstudio.com/)
    * This isn't required but is quite handy to use if you will be editing configuration files or developing code.

## Installing the tools
If you don't already have VScode installed on your machine please visit the following website and follow the install directions:
<https://code.visualstudio.com/>

If you'll be developing code I suggest you also install the [C/C++ extensions](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools)

For the remaining tools I recommend installing them into into a single folder. On my Windows machine I use `C:\Tools`. You could also choose to use `C:\VSARM`. Keep it simple and avoid having spaces in the path. 

On a Mac or Linux machine you could create a subfolder in your home folder.

```
mkdir ~/vsarm
```

### Install arm gnu toolchain

Go to: <https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads>
Download the appropriate installer. There are several available. 

#### Windows installation
For my Windows development PC I chose:

```
gcc-arm-none-eabi-8-2019-q3-update-win32-sha2.exe
Windows 32-bit Installer (Signed for Windows 7 and later)
MD5: d44f44b258b203bdd6808752907754be
```

Run the installer. When it prompts you to "Choose Install Location" I changed the default shown in "Destination Folder"
from:

```
C:\Program Files (x86)\GNU Tools ARM Embedded\8 2019-q3-update
```
to 
```
C:\Tools\armcc
```

Make sure these gnu tools are in your path. More specifically you will want to add this to your path:
```
C:\Tools\armcc\bin
```

#### Mac OS installation

For my old MacBook I chose to download this version:

```
Mac OS X 64-bit
File: gcc-arm-none-eabi-8-2019-q3-update-mac.tar.bz2 (105.72 MB)
```

Using a Finder window I then double-clicked on the downloaded `.bz2` file which unpacked the contents into a new folder named:

```
gcc-arm-none-eabi-8-2019-q3-update
```

I then renamed that folder to `armcc` and then moved this `armcc` folder into my `~/vsarm` folder.

Finally I adjusted my path by editing my `.zshrc` file and added this to the bottom of it:

```
# vsarm tools
export PATH="$HOME/vsarm/armcc/bin:$PATH"

```

> Note: If you use `bash` for your shell (which is the default for Mac OS) then add the above to your `~/.bash_profile` instead.


### Install MinGW-W64 (for Windows machines only)

This step is only needed for Windows development machines.

We need MinGW-W64 for the `mingw32-make.exe` program. If you have already have a `make` utility on your machine then you could skip this

Go to: <https://sourceforge.net/projects/mingw-w64/>
Change the install location to:

```
C:\Tools\mingw
```

Make sure the MinGW-W64 tools are in your path. More specifically you will want to add this to your path:
```
C:\Tools\mingw\mingw32\bin
```

### Installing OpenOCD on Windows

Again, this is only necessary if you wish (and are able) to use the ST-Link adapter.

For Windows users I recommend obtaining a version of OpenOCD from here: <https://gnutoolchains.com/arm-eabi/openocd/>

Download the most recent 7zip file: `openocd-20190828.7z`
Decompress it into `C:\Tools` folder. Rename the resultant `OpenOCD-20190828-0.10.0` folder to just `OpenOCD`.

Make sure the OpenOCD tools are in your path. More specifically you will want to add this to your path:

```
C:\Tools\OpenOCD\bin
```

> Note: There are several binary versions of OpenOCD available for installation. The one from SysProgs (gnutoolchains.com)
seems to be updated regularly and also provides drivers that may be helpful to you.

### Installing OpenOCD on Mac OS
While it may be possible to install a prebuilt version of OpenOCD using Homebrew, I've learned that 
the OpenOCD project maintainers recommend Mac/Linux users to build it themselves using the latest version
of the source code available from the repository. 

We'll use [Homebrew](https://brew.sh/) to ***build*** and install OpenOCD using the latest available source code. Using Terminal, enter the following:

```
brew install open-ocd --HEAD
```

This took a little while to complete, but once it finished I found it was immmediately available for use at `/usr/local/bin/openocd`. No adjustments to my path were needed.

### Configure VSCode shell

**One very important note:** The makefile I'm using uses `rm -rf` as part of building the `clean` target. This `rm` command isn't normally available on windows. So I've configured my VSCode environment to use a `bash` shell whenever it needs to provide a shell for any command.

This `bash` shell came with my [Git for Windows](https://gitforwindows.org/) installation. If you have something similar, then you'll want to configure VSCode to use such a shell. Alternatively you can edit the makefile(s) in this project folder.

To configure VSCode to use a particular shell you need to edit your `settings.json` file. Details are provided here: <https://code.visualstudio.com/docs/editor/integrated-terminal>. 

Basically you need to enter a configuration line like ***one*** of the following:

```json
// Command Prompt
"terminal.integrated.shell.windows": "C:\\Windows\\System32\\cmd.exe"
// PowerShell
"terminal.integrated.shell.windows": "C:\\Windows\\System32\\WindowsPowerShell\\v1.0\\powershell.exe"
// Git Bash
"terminal.integrated.shell.windows": "C:\\Program Files\\Git\\bin\\bash.exe"
// Bash on Ubuntu (on Windows)
"terminal.integrated.shell.windows": "C:\\Windows\\System32\\bash.exe"
```

Obviously you should use one of the `bash` options in the above examples. Personally I prefer the `bash` from my `git` install versus the one from my Windows Subsystem for Linux (WSL) since it loads up much faster.

