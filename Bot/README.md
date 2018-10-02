# Antibiotic bot
Antibiotic is an anti-virus program for embedded devices connected to the internet. It will protect devices by periodically scanning for known malware and also kill programs listening on ports that are known to be abused to gain unauthorized access to devices.

The source code is adapted from the Mirai source code and heavily modified.

The `src` directory contains the source code for the Antibiotic bot. The `tools` directory contains various tools to help development of Antibiotic. In the top of all file in both directories is a comment explaining its functionality.

Note that this is a proof-of-concept and should not be used in production. This proof-of-concept specifically targets the router named Netgear DGN1000.

# Commands
The Antibiotic bot can communicate with a server and currently the following commands are implemented.

* **Add new pattern.** Patterns are used to match against when scanning for malware.
* **Clear all patterns.**
* **Add new port.** Attempt to kill all programs listening on the specified port and bind to the port so it can't be used again.
* **Change scanning interval.** Changes the time between malware scans.
* **Change web interface password.** Change the password of the routers web interface.
* **Get report status.** Evaluate reports created when malware is found and send result to server.
* **Reboot device.**
* **Exit.**

# Building
* Setup Buildroot
    * Download and extract from https://buildroot.org/download.html
    * Run `make menuconfig` inside the Buildroot folder
        * Important options are "Target options -> Target Architecture" and "Toolchain -> C library"
        * The tested configuration is "MIPS (big endian)" and "musl"
    * Run `make` in the Buildroot folder
* Edit `build.sh`, the default build script contains an example setup for mips
* Run `./build.sh debug` or `./build.sh release` to build Antibiotic

# Code style
The LLVM Coding Standards is used for the C code.
Format the code by running `clang-format -i -style=LLVM *.c *.h` inside the `src` directory.

# Acknowledgment
The first version of this AntibIoTic bot module was created as a part of the Bachelor's thesis "Bot module for AntibIoTic" (2018) by Mathies Svarrer-Lanthén, under the supervision of Michele De Donno (PhD student) and Nicola Dragoni (Associate Professor) at DTU, Technical University of Denmark (https://github.com/seihtam/antibiotic).
