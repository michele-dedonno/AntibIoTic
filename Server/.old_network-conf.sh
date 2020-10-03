#! /bin/bash
# Helper script that provides the network configuration for the Fog Node of AntibIoTic
# NOTE: the scripts needs to be executed with sudo priviledges

###### DEPRECATED ######
# This script was used in previous versions of AntibIoTic, and it is kept only as a reference. 
# This script is now DEPRECATED and should not be used.
#######################

# ------------- Variables to be edited -------------
out_int="wlp2s0"
in_int1="enx086d41e55185"
in_int2="enp0s31f6"
ipset_name="allowed_ips"
ipset_type="iphash"
# ------------- --------------------- -------------#


echo "[script]> Configuring /etc/network/interfaces ..."


# --------- Input Interfaces --------- #
# [Input interface 1]
grep "auto $in_int1" /etc/network/interfaces > /dev/null
status=$? # stores exit status of grep: 0 if found something
if ((status!=0)); then
        printf "\n# start the interface at boot" >> /etc/network/interfaces
	printf "\nauto %s" "$in_int1" >> /etc/network/interfaces
fi


grep "iface $in_int1" /etc/network/interfaces > /dev/null
status=$?
if ((status!=0)); then
	#strg="iface $in_int1 inet static"
	#sudo bash -c "echo $strg >> /etc/network/interfaces"
	printf "\n# IP configuration for the ethernet interface" >> /etc/network/interfaces
	printf "\niface %s inet static" "$in_int1" >> /etc/network/interfaces
	printf "\n\taddress 192.168.0.2" >> /etc/network/interfaces
	printf "\n\tnetmask 255.255.255.0" >> /etc/network/interfaces
	printf "\n\tgateway 192.168.0.2\n" >> /etc/network/interfaces
fi


# [Input interface 2]
grep "auto $in_int2" /etc/network/interfaces > /dev/null
status=$? # stores exit status of grep: 0 if found something
if ((status!=0)); then
        printf "\n# start the interface at boot" >> /etc/network/interfaces
	printf "\nauto %s" "$in_int2" >> /etc/network/interfaces
fi


grep "iface $in_int2" /etc/network/interfaces > /dev/null
status=$?
if ((status!=0)); then
	#strg="iface $in_int2 inet static"
	#sudo bash -c "echo $strg >> /etc/network/interfaces"
	printf "\n# IP configuration for the ethernet interface" >> /etc/network/interfaces
	printf "\niface %s inet static" "$in_int2" >> /etc/network/interfaces
	printf "\n\taddress 192.168.0.2" >> /etc/network/interfaces
	printf "\n\tnetmask 255.255.255.0" >> /etc/network/interfaces
	printf "\n\tgateway 192.168.0.2\n" >> /etc/network/interfaces
fi

echo "[script]> Enabling packet forwarding..."
# enable packet forwarding
sysctl net/ipv4/ip_forward=1

echo "[script]> Creating the ipset..."
# create an ipset (used for keeping track of devices allowed to the Internet)
ipset create $ipset_name $ipset_type

echo "[script]> Configuring iptables..."
# %-------------------%
#  Configure iptables
# %-------------------%
iptables -P INPUT ACCEPT
iptables -P FORWARD DROP 
iptables -P OUTPUT ACCEPT

# %-- Delete potentially existing rules to avoid duplicates --%
iptables -t nat -D POSTROUTING -o $out_int -j MASQUERADE
iptables -D FORWARD -i $in_int1 -o $out_int -m set --match-set $ipset_name src -j ACCEPT
iptables -D FORWARD -i $in_int2 -o $out_int -m set --match-set $ipset_name src -j ACCEPT
iptables -D FORWARD -i $out_int -o $in_int1 -m state --state RELATED,ESTABLISHED -j ACCEPT 
iptables -D FORWARD -i $out_int -o $in_int2 -m state --state RELATED,ESTABLISHED -j ACCEPT 
iptables -D FORWARD

# %-- Add rules --%
# enable Dynamic NAT
iptables -t nat -A POSTROUTING -o $out_int -j MASQUERADE 
# enable in->out fowarding only for hosts in the ipset
iptables -A FORWARD -i $in_int1 -o $out_int -m set --match-set $ipset_name src -j ACCEPT
iptables -A FORWARD -i $in_int2 -o $out_int -m set --match-set $ipset_name src -j ACCEPT
# enable out->in forwarding for established connections
iptables -A FORWARD -i $out_int -o $in_int1 -m state --state RELATED,ESTABLISHED -j ACCEPT 
iptables -A FORWARD -i $out_int -o $in_int2 -m state --state RELATED,ESTABLISHED -j ACCEPT 

echo "[script]> iptables:"
echo "-------------------------------------------------"
# show current iptables rules (use -L for table format)
iptables -S
echo "----------------- NAT ---------------------------"
iptables -t nat -S
echo "-------------------------------------------------"
echo "[script]> Please note that only packets from IPs in the ipset '$ipset_name' are forwarded. if you want to add an IP to the set use: 'sudo ipset add $ipset_name <IP>'. To check details currently setting of ipset you can use 'sudo ipset -L'."
echo "[script]> ipsets:"
echo "-------------------------------------------------"
ipset -L
echo "-------------------------------------------------"
