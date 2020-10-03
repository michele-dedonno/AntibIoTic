# Antib*IoT*ic Gateway
This folder contains everything needed to run the AntibIoTic Gateway.

## Configuration
This section contains the configuration needed to setup the Fog node as the network gateway running AntibIoTic. The configuration needs to be double-checked at each reboot, especially the part related to iptables.

Note: it is assumed that the Fog Node has a working Internet connection via Wi-Fi (interface `wlp2s0`) and that Go is installed on the system.

*This configuration was tested on `Ubuntu 18.04.5 LTS (Bionic Beaver)`, kernel version `Linux 4.15.0-112-generic`, Go version `go1.15.1 linux/amd64`.*

To compare with the details of your system you can use the following commands:
```bash
# Linux distribution
cat /etc/*-release
# Kernel version
uname -a
# Go version
go version
```


### Automated
The Fog node network configuration can be executed automatically by running the helper script [./network-conf-netplan.sh](./network-conf-netplan.sh) with `sudo` priviledges at each reboot.
```bash
sudo ./network-conf-netplan.sh
```

### Manual

1. Configure the network interfaces as follows.
    - Insert in `/etc/netplan/01-network-manager-all.yaml` the following network configuration:
        ```bash
        network:
                version: 2
                renderer: networkd
                ethernets:
                        # --- USB-Eth adapter connected to UDOOx86 --- #
                        enx086d41e55185:
                                dhcp4: no
                                addresses: [192.169.0.2/24]
                        # --- USB-Eth adapter connected to Raspberry Pi 3 --- #
                        enx086d41e63ac6:
                                dhcp4: no
                                addresses: [192.170.0.2/24]
                        # --- USB-Eth adapter connected to NETGEAR Router --- #
                        enx5cf7e68b618b:
                                dhcp4: no
                                addresses: [192.168.0.2/24]
        ```
    - Test the syntax of your netplan with: `sudo netplan try`
    - If no errors are raised, apply the configuration: `sudo netplan apply`

2. Allow the forwarding of IPv4 packets:

```shell
sudo sysctl net/ipv4/ip_forward=1
```

3. We also need to keep track of which device is allowed to access the Internet. For this purpose, we created the `allowed_ips` IP set.

```shell
sudo ipset n allowed_ips iphash
```

4. Finally, we add the iptables rules that will allow only the IPs in the `allowed_ips` group to access the Internet, routing the traffic from the ethernet link to the the Wi-Fi (and vice-versa).

```shell
# Enable NAT for out-going traffic of the Wi-Fi interface
sudo iptables -t nat -A POSTROUTING -o wlp2s0 -j MASQUERADE
# Allow in-going and out-going traffic
sudo iptables -P INPUT ACCEPT
sudo iptables -P OUTPUT ACCEPT
# Block all forwarded traffic
sudo iptables -P FORWARD DROP
# Allow to forward outgoing traffic only for hosts in the allowed ipset group
sudo iptables -A FORWARD -o wlp2s0 -m set --match-set allowed_ips src -j ACCEPT
# Allow incoming traffic for established connections
sudo iptables -A FORWARD -i wlp2s0 -m state --state RELATED,ESTABLISHED -j ACCEPT
```

If you want internal traffic to be forwarded within the LAN (e.g., ICMP local requests), add also the following rule:
```bash
sudo iptables -A FORWARD -s 192.168.0.0/14 -d 192.168.0.0/14 -j ACCEPT
```
## Required Files
Before running the server, make sure that all files required to upload the AntibIoTic Agent on the IoT devices are located in [./.upload/](./.upload).
The helper script [setup-upload.sh](./setup-upload.sh) can be used to automate this step.

*Netgear DGN1000 Router* requires:
- [upload.py](../Agent/netgear-mips/upload.py) located in `./.upload/netgear/`
- agent binary file `antibiotic_debug_mips` located in `./.upload/netgear/`.

*Udoo x86 II Advanced Plus* requires:
- [upload.py](../Agent/udoo-x86/upload.py) located in `./.upload/udoo/`;
- agent src files in the folder `./.upload/udoo/src/`.

*Raspberry Pi 3 Module B+* requires:
- [upload.py](../Agent/raspberry-arm/upload.py) located in `./.upload/raspberry/`;
- agent src files in the folder `./.upload/raspberry/src/`. 

## Compilation and Execution

### Automated
To automatically compile the server, the helper script [./build.sh](./build.sh) can be executed.

### Manual
To compile the server, the following needs to be executed in the current folder:

```bash
export GOPATH=$(pwd)

# Create bin folder
mkdir -p bin

# Install dependencies
go get golang.org/x/crypto/ssh
go get golang.org/x/sys/unix

# Compile the code
go build -o bin/gateway gateway
```

---

The server binary will be created in the newly created `bin` folder.

Now, to run the AntibIoTic Gateway, execute the binary with sudo priviledges: `sudo ./bin/gateway`
If you want to run the Gateway without automatically uploading the Agent on each IoT device, use the following command: `sudo ./bin/gateway -u`.

## Troubleshooting

- **One of the server interfaces is in a "NO CARRIER" state and does not get the IP address it should**
    - make sure that the cable is properly plugged in for both the peer-device socket and the server socket;
    - reboot the peer-device while the server is on so that it can detect the peer-device connection.
