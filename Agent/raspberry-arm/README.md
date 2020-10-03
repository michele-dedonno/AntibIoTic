# Content
This folder contains everything needed to run the AntibIoTic Agent on a Raspberry Pi 3 Module B+ or similar ARM devices.

*Tested with ` Raspbian GNU/Linux 9` kernel version `Linux 4.19.66-v7+`*

## Configuration
1. Add the following network configuration in `/etc/network/interfaces`:
```
# start interface at boot
auto eth0
# IP configuration for the interface
iface eth0 inet static
    address 192.170.0.1
    netmask 255.255.255.0
    gateway 192.170.0.2
```
2. Enable ssh at boot: `sudo systemctl enable ssh`

3. Start ssh: `sudo systemctl start ssh`

## Execution

### Automated
To automatically compile and run the AntibIoTic Agent on a Raspberry Pi 3 Module B+ located at the IP 192.170.0.1 the [./upload.py](./upload.py) helper script can be used as follows:
```bash
python3 ./upload.py -r 192.170.0.1 -s 192.170.0.2 -f ../src -x
```
Note: it is assumed that the command is executed from the directory where this README file is located.

### Manual
To manually compile and execute the AntibIoTic Agent on a Raspberry Pi 3 Module B+ follow the instructions below.

Note: it is assumed that the following commands are executed from the directory where this README file is located.

1. Upload source files to remote host 192.170.0.1:
```bash
scp -r ../src pi@192.170.0.1:antibiotic_src/
```
2. Connect via SSH:
```bash
ssh pi@192.170.0.1
```
3. Organize folders and compile the code in DEBUG mode:
```bash
mkdir -p AntibIoTic/bin
mv antibiotic_src/ AntibIoTic/src
gcc AntibIoTic/src/*c -o AntibIoTic/bin/antibiotic_pi_debug -std=c99 -pthread -g -DDEBUG
// If you want all warnings, use:
// gcc AntibIoTic/src/*c -o AntibIoTic/bin/antibiotic_pi_debug -Wall -std=c99 -pthread -g -DDEBUG
```
4. Execute the Agent with device ID = 3 and Gatewy IP = 192.170.0.2:
```bash
chmod +x AntibIoTic/bin/antibiotic_pi_debug
./AntibIoTic/bin/antibiotic_pi_debug 192.170.0.2 3
```
## Compile and Run Tests
```bash
mkdir -p ./AntibIoTic/bin/tests
gcc ./AntibIoTic/src/tests/<test-name>/*.c -o ./AntibIoTic/bin/tests/<test-name> -Wall -std=c99 -pthread -g -DDEBUG
./AntibIoTic/bin/tests/<test-name>
