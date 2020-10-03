package main

import (
	"encoding/binary"
	"fmt"
	"io"
	"errors"
	"os"
	"net"
	"os/exec"
	"strings"
	"time"
)
const RCV_DEADLINE = 7 // seconds waiting for incoming data before unloking the READ

/*
Network functions
*/

func send(conn net.Conn, cmd byte, data []byte) (err error) {
	// Send data size
	if data != nil {
		err = binary.Write(conn, binary.BigEndian, uint16(len(data)))
	} else {
		err = binary.Write(conn, binary.BigEndian, uint16(0))
	}
	if err != nil {
		fmt.Println(err)
		return
	}

	// Send data type
	_, err = conn.Write([]byte{cmd})
	if err != nil {
		return
	}

	// Send data
	if data != nil {
		_, err = conn.Write(data)
		if err != nil {
			return
		}
	}

	return
}

func receive(conn net.Conn) (dataType byte, data []byte, err error) {
	// Set RCV_DEALINE seconds deadline for read operations
	conn.SetReadDeadline(time.Now().Add(RCV_DEADLINE*time.Second))

	// Receive size
	var size uint16
	err = binary.Read(conn, binary.BigEndian, &size)
	if err != nil {
		// handle timeout error
		if errors.Is(err, os.ErrDeadlineExceeded){
			return 0xff, nil, nil
		}
		return
	}

	dataReceived := make([]byte, size+1)

	// Receive data and date type
	_, err = io.ReadFull(conn, dataReceived)
	if err != nil {
		return
	}

	return dataReceived[0], dataReceived[1:], err
}

// Takes an ipaddress and a port in format "n1.n2.n3.n4:portNo" and returns "n1.n2.n3.n4"
func extractIpAddress(ipAddressAndPort string) string {
	parts := strings.Split(ipAddressAndPort, ":")
	return parts[0]
}

// removes ipAddress from the allowed_ips ipset
func blockIPAddress(ipAddress string) {
	exec.Command("ipset", "del", "allowed_ips", ipAddress).Run()
	fmt.Println("[Handler]> Blocking IP address", ipAddress)
}

// adds ipAddress from the allowed_ips ipset
func allowIPAddress(ipAddress string) {
	exec.Command("ipset", "add", "allowed_ips", ipAddress).Run()
//	fmt.Println("[Handler]> Allowing IP address", ipAddress)
}

// function used to check if an IP is already in the ipset
// Return: true if IP is in the set, false otherwise
func checkIP(ipAddress string) bool {
	out, err := exec.Command("ipset","test", "allowed_ips", ipAddress).Output()
	if err != nil {
		return false
	}
	return !strings.Contains(string(out), "NOT")
}

// remove all IPs in the allowed_ips ipset
func clearIPset(){
	err := exec.Command("ipset","flush","allowed_ips").Run()
	if err != nil{
		fmt.Println("Error while initializing the ipset: ",err)
	}
}
// debug function to quickly show the iptables rules and the allowed_ips ipset
func showIPRules() {
	fmt.Println("-----------------------------")
	out, _ := exec.Command("iptables", "-L").Output()
	fmt.Println(string(out))
	fmt.Println("--------------")
	out, _ = exec.Command("ipset", "list", "allowed_ips").Output()
	fmt.Println(string(out))
	fmt.Println("-----------------------------")
}
