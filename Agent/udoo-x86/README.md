# Content
This folder contains everything needed to run the AntibIoTic Agent on an UDOO x86 II ADVANCED PLUS board or similar x86 devices.

*Tested with `Ubuntu 20.04 LTS` kernel version `Linux 5.4.0-37-generic`*

## Configuration
1. Add the following configuration in `/etc/netplan/01-network-manager-all.yaml`:
```
network:
    version: 2
    renderer: networkd
    ethernets:
        enp2s0:
            dhcp4: no
            addresses: [192.169.0.1/24]
            gateway4: 192.169.0.2
```
2. `sudo netplan try`
3. If there are no errors: `sudo netplan apply`

## Execution

### Automated
To automatically compile and run the AntibIoTic Agent on a UDOO x86 board located at the IP 192.169.0.1 the [./upload.py](upload.py) helper script can be used as follows:
```bash
python3 ./upload.py -r 192.169.0.1 -u udoo -s 192.169.0.2 -f ../src -x
```
Note: it is assumed that the command is executed from the directory where this README file is located.

### Manual
To manually compile and execute the AntibIoTic Agent on a UDOO x86 board follow the instructions below.

Note: it is assumed that the following commands are executed from the directory where this README file is located.

1. Upload source files to remote host 192.169.0.1:
```bash
scp -r ../src udoo@192.169.0.1:antibiotic_src/
```
2. Connect via SSH:
```bash
ssh udoo@192.169.0.1
```
3. Organize folders and compile the code in DEBUG mode:
```bash
mkdir -p AntibIoTic/bin
mv antibiotic_src/ AntibIoTic/src
gcc AntibIoTic/src/*c -o AntibIoTic/bin/antibiotic_udoo_debug -std=c99 -pthread -g -DDEBUG
// If you want all warnings, use:
// gcc AntibIoTic/src/*c -o AntibIoTic/bin/antibiotic_udoo_debug -Wall -std=c99 -pthread -g -DDEBUG
```
4. Execute the Agent with device ID = 2 and Gateway IP = 192.169.0.2:
```bash
chmod +x AntibIoTic/bin/antibiotic_pi_debug
./AntibIoTic/bin/antibiotic_pi_debug 192.169.0.2 2
```

### Compile and Run Tests
```bash
mkdir -p ./AntibIoTic/bin/tests
gcc ./AntibIoTic/src/tests/<test-name>/*.c -o ./AntibIoTic/bin/tests/<test-name> -Wall -std=c99 -pthread -g -DDEBUG
./AntibIoTic/bin/tests/<test-name>
```
