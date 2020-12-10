# Antib*IoT*ic Agent
This folder contains everything related to the Antibiotic Agent.
In details:
- [./src](./src): directory containing the source code; 
- [./udoo-x86](./udoo-x86): directory containing everything needed to run the AntibIoTic agent on the UDOO x86 II ADVANCED PLUS board or similar devices.
- [./raspberry-arm](./raspberry-arm): directory containing everything needed to run the AntibIoTic agent on Raspberry Pi 3 Module B+ or similar devices;
- [./netgear-mips](./netgear-mips): directory containing everything needed to run the AntibIoTic agent on NETGEAR Router DGN1000 or similar devices.

Each folder has a README file with further instructions.

## Compilation
Depending on the device where the Agent needs to be executed, different instructions need to be followed. 
Please refer to the folder that best suits the target architecture:
- x86: [./udoo-x86](./udoo-x86);
- ARM: [./raspberry-arm](./raspberry-arm);
- MIPS: [./netgear-mips](./netgear-mips).

## Usage
Once compiled, the Agent can be executed as follows:
```
./antibiotic_agent [<server-IP> <device-ID>]
```
If no arguments are specified, the default server IP and a random device ID will be used.

## Commands
The Antibiotic Agent communicates with the AntibIoTic Gateway and currently implements the following remote commands that can be executed when requested from the Gatweway:
* **Add new signature.** Patterns are used to match against when scanning for malware.
* **Clear all signatures.**
* **Add new port.** Attempt to kill all programs listening on the specified port and bind to the port so it can't be used again.
* **Change scanning interval.** Changes the time between malware scans.
* **Change web interface password.** Changes the password of the routers web interface.
* **Get report status.** Evaluates the local report (log) created and sends it to the server.
* **Reboot device.**
* **Exit.**

Please note that depending on the device running the Agent some commands might not work or be not available.

## Code style
The LLVM Coding Standards is used for the C code.

You can format the code by running `clang-format -i -style=LLVM *.c *.h` inside the `src` directory.

