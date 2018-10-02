package main

/*
AntibIoTic server.
Can send command to client and receive keep-alive messages.
This is a proof of concept to test AntibIoTic.
*/

import (
	"bufio"
	"encoding/binary"
	"fmt"
	"io"
	"net"
	"os"
	"strings"
)

func check(e error) {
	if e != nil {
		panic(e)
	}
}

func main() {
	// Listen on all interfaces
	ln, err := net.Listen("tcp", ":1234")
	check(err)

	defer ln.Close()
	fmt.Println("Listening on", ln.Addr())

	// Accept connection
	fmt.Println("Waiting for connection...")
	conn, err := ln.Accept()
	check(err)

	defer conn.Close()
	fmt.Println("Connected to", conn.RemoteAddr())

	commands := map[string]byte{
		"HTTP_PASSWORD":  byte(1),
		"REBOOT":         byte(2),
		"PATTERN":        byte(3),
		"PORT":           byte(4),
		"SCAN_INTERVAL":  byte(5),
		"EXIT":           byte(6),
		"CLEAR_PATTERNS": byte(7),
		"REPORT_STATUS":  byte(8),
	}

	sampleData := map[string][]byte{
		"HTTP_PASSWORD":  []byte{'a', 'b', 'c'},
		"REBOOT":         nil,
		"PATTERN":        []byte{0x0C, 0x48, 0x4C, 0x00, 0x4F, 0x33, 0x22, 0x77, 0x77},
		"PORT":           []byte{0x7F, 0xFC}, // 32764 in network byte order
		"SCAN_INTERVAL":  []byte{0x00, 0x14}, // 20 in network byte order
		"EXIT":           nil,
		"CLEAR_PATTERNS": nil,
		"REPORT_STATUS":  nil,
	}

	// Start keep alive receiver
	go keepAliveReceiver()

	// Start command receiver
	go commandReceiver(conn)

	// Send commands to client
	reader := bufio.NewReader(os.Stdin)
	for {
		fmt.Print("Command type: ")
		str, err := reader.ReadString('\n')
		check(err)
		str = strings.TrimSuffix(str, "\n")

		cmd, ok := commands[str]
		if !ok {
			fmt.Println("Invalid command type")
			continue
		}

		err = send(conn, cmd, sampleData[str])
		check(err)
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
	//fmt.Println("Sending data and type:", cmd)
	_, err = conn.Write([]byte{cmd})
	if err != nil {
		return
	}

	// Send data
	if data != nil {
		//fmt.Println("Sending data...")
		_, err = conn.Write(data)
		if err != nil {
			return
		}
	}

	return
}

func receive(conn net.Conn) (dataType byte, data []byte, err error) {
	// Receive size
	//fmt.Print("Receiving data size... ")
	var size uint16
	err = binary.Read(conn, binary.BigEndian, &size)
	if err != nil {
		return
	}

	dataReceived := make([]byte, size+1)

	// Receive data and date type
	//fmt.Println("Receiving data...")
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
		defer ln.Close()

		// Accept connection
		conn, err := ln.Accept()
		check(err)
		defer conn.Close()

		// Receive keep-alive messages
		for {
			msg := make([]byte, 2)

			_, err = io.ReadFull(conn, msg)
			if err != nil {
				fmt.Println("Keep alive socket err:", err)
				break
			}

			fmt.Println("\nReceived keep-alive message")
		}
	}
}

func commandReceiver(conn net.Conn) {
	for {
		cmd, data, err := receive(conn)
		check(err)
		fmt.Printf("\nReceived command %x with data: %s\n", cmd, data)
	}
}
