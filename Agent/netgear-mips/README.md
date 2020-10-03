# Content
This folder contains everything needed to run the AntibIoTic Agent on a NETGEAR Router DGN1000 or similar MIPS devices.

## Configuration
In the NETGEAR Router DGN 1000, the operating system is loaded from an .img file every time it boots up. Consequently, it is not possible to store system parameters in files, except for a couple developed by Netgear. It is also not possible to directly configure the router to have the Fog node as its default gateway.

However, exploiting the [remote command execution vulnerability](https://www.exploit-db.com/exploits/25978) in the Netgear router, it is possible to configure the Fog node as default gateway using the helper script [./shell.py](./shell.py):
```bash
python3 ./shell.py
route add default gw 192.168.0.2 br0
```
Note: it is assumed that the command is executed from the directory where this README file is located.

## Building the AntibIoTic Agent
1. Setup Buildroot
    * Download and extract from https://buildroot.org/download.html (tested with buildroot-2018.02.7): `wget https://buildroot.org/downloads/buildroot-2018.02.7.tar.gz && tar xvf buildroot-2018.02.7.tar.gz && rm buildroot-2018.02.7.tar.gz`;
    * Run `make menuconfig` inside the Buildroot folder:
        * Important options are "Target options -> Target Architecture" and "Toolchain -> C library";
        * The tested configuration is "MIPS (big endian)" and "musl";
    * Run `make` in the Buildroot folder;
2. Edit `build.sh` to adapt paths variable to your setting. The default build script contains an example setup for mips, assuming the directory tree of this repository;
3. Run `./build.sh debug` or `./build.sh release` to build the AntibIoTic Agent.

## Execution
The Agent code is executed on the router using the script [./upload.py](./upload.py).
An example of command that can be used to run the Agent:
```bash
python3 ./upload.py -r 192.168.0.1 -l 192.168.0.2 -s 192.168.0.2 -f ./bin/antibiotic_debug_mips -n antibiotic -x -e
```
Once the Agent is executed on the remote device, the script [./shell.py](./shell.py) can be used to monitor its execution.
Alternatively, the script [./exec_cmd.py](`./exec_cmd.py`) can be used to run a single command on the router.

Some useful commands:
* check if the AntibIoTic Agent is running:
```bash
ps
```
* stop the execution of the Agent:
```bash
killall antibiotic
```
* show debug information:
```bash
cat tmp/debug.txt
```
* show the report generated so far:
```bash
cat tmp/report.txt
```
# Compile and Run Tests
```bash
./build.sh test <test-folder>
python3 ./upload.py -r 192.168.0.1 -l 192.168.0.2 -f ./bin/tests/antibiotic_<test-folder>_test_mips -n <test-name> -e -nx

```
