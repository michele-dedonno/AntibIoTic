package main

/*
AntibIoTic server.
Can send commands to clients and receive keep-alive messages.
This is a proof of concept to test AntibIoTic.
*/

import (
	"fmt"
	"io"
	"net"
	"os/exec"
	"syscall"
	"strings"
	"flag"

	"golang.org/x/crypto/ssh/terminal"
)

// type that symbolizes the configuration of a device, holding its name, IP and the
// necessary command and arguments to upload and run the agent
type deviceConfig struct {
	name    string
	ip      string
	command string
	args    []string
}

const REPORT_PATH = "/tmp/reports/"

func main() {
	var noUpload = flag.Bool("u", false, "If set, the Agent will not be uploaded on IoT devices at startup")
	flag.Parse()

	// remove all IPs in the allowed ipset
	clearIPset()

	// check flag
	if *noUpload == false {
		// Get SSH passwords for UDOO and Raspberry
		pass1 := getPassword("UDOO x86", "SSH")
		pass2 := getPassword("Raspberry Pi", "SSH")

		devices := []deviceConfig{
			deviceConfig{
				name:   "Netgear DGN1000",
				ip:     "192.168.0.1",
				command:"python3",
				args:   []string{"./.upload/netgear/upload.py", "-r", "192.168.0.1", "-l", "192.168.0.2", "-f", "./.upload/netgear/antibiotic_debug_mips", "-n", "antibiotic", "-x", "-e"},
			},
			deviceConfig{
				name:	"UDOO x86 II Advanced Plus",
				ip:	"192.169.0.1",
				command: "python3",
				args:	[]string{"./.upload/udoo/upload.py", "-r", "192.169.0.1", "-u", "udoo","-pw", pass1, "-f", "./.upload/udoo/src", "-x", "-s", "192.169.0.2"},
			},
			deviceConfig{
				name:	"Raspberry Pi 3 Module B+",
				ip:	"192.170.0.1",
				command: "python3",
				args:	[]string{"./.upload/raspberry/upload.py", "-r", "192.170.0.1", "-u", "pi","-pw", pass2, "-f", "./.upload/raspberry/src", "-x", "-s", "192.170.0.2"},
			},
		}

		// Load the Agent on each IoT device
		Loader(devices)
	}else{
		fmt.Println("[Handler]> The Agent will not be automatically uploaded on IoT devices.")
	}

	// Start receiving keep-alive messages
	go Spotter()

	// Listen on all interfaces, port 1234
	ln, err := net.Listen("tcp", ":1234")
	check(err, "[Handler]> Error while listening on all interfaces")
	fmt.Println("[Handler]> Listening on", ln.Addr())
	defer ln.Close()
	for {
		// Accept connection
		// fmt.Println("Waiting for connection...")
		conn, err := ln.Accept()
		check(err, "[Handler]> Error while accepting the connection")
		fmt.Println("[Handler]> New connection established:", conn.RemoteAddr())
		// Start receiving messages
		go requestsHandler(conn)

	}
}

// Load the agent on each device
func Loader(devices []deviceConfig){

	for _, device := range devices{
		// upload and run the bot
		fmt.Printf("[Loader]> Uploading and running the agent on device %s (%s)\n", device.name, device.ip)
		//fmt.Println(device.command, strings.Join(device.args, " "))
		go uploadAndRunBot(device.command, device.args) // Upload, execute, and hangs
	}
}

// Handle connections for keep-alive messages
func Spotter() {
	// Listen on all interfaces. port 4321
	ln, err := net.Listen("tcp", ":4321")
	if check(err, "[Spotter]> Error while listening on all interfaces") == 1 {
		return
	}
	fmt.Println("[Spotter]> Listening on", ln.Addr())
	defer ln.Close()
	for{
		// Accept connection
		conn, err := ln.Accept()
		if check(err, "[Spotter]> Error while accepting the connection") == 1 {
			continue
		}
		// Receive keep-alive messages
		go KeepAliveReceiver(conn)
	}
}

// Receive keep-alive messages
func KeepAliveReceiver(conn net.Conn){

	for {
		msg := make([]byte, 2)

		_, err := io.ReadFull(conn, msg)
		if err != nil {
			//fmt.Println("[Spotter]> socket err:", err)
			// if connection is severed, block IP address
			ipAddress := extractIpAddress(conn.RemoteAddr().String())
			blockIPAddress(ipAddress)
			break
		}
		fmt.Println("[Spotter]> '",conn.RemoteAddr(),"': keep-alive message")
		// if not already allowed, allow IP address
		ipAddress := extractIpAddress(conn.RemoteAddr().String())
		if !checkIP(ipAddress){
			allowIPAddress(ipAddress)
		}
	}
	conn.Close()
}

// Return 1 in case of error, 0 otherwise
func check(e error, msg string)(int) {
	if e != nil {
		fmt.Print("*** ", msg, ". '", e, "' ***\n")
		// panic(e)
		return 1
	}
	return 0
}

// Get password from user
func getPassword(device string, service string)(string){
	fmt.Printf("Enter %s password for device %s:\n", service, device)
	bytePassword, err := terminal.ReadPassword(int(syscall.Stdin))
	check(err, "Error while reading the password")
	//fmt.Println("\nPassword typed: " + string(bytePassword))

	return string(bytePassword)
}

// Executes the command to upload and run the agent to one device
// Return 1 if fails, 0 otherwise
func uploadAndRunBot(command string, args []string) (int){
	err := exec.Command(command, args...).Run()
	msg := "[Loader]> Error while uploading and running the agent: " + command +" "+ strings.Join(args, " ")
	return check(err, msg)
}
