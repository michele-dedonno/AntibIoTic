package main

/*
Request handler
In charge of receiving and sending requests
*/

import (
	"encoding/binary"
	"fmt"
	"io/ioutil"
	"os/exec"
	"net"
	"time"
	"strings"
)

const REPORTS_TIME_INTERVAL = 60 // time interval (in secs) between Report requests to Agents

const RECEIVE_ID = 0x01
const RECEIVE_VERSION = 0x02
const RECEIVE_REPORT = 0x03
const RECEIVE_UPDATE = 0x04
const RECEIVE_SUCCESS = 0x88
const RECEIVE_ERROR = 0x99

const CMD_SET_HTTP_PASSWORD = 0x01
const CMD_REBOOT = 0x02
const CMD_ADD_PATTERN = 0x03
const CMD_ADD_PORT = 0x04
const CMD_SET_SCAN_INTERVAL = 0x05
const CMD_EXIT = 0x06
const CMD_CLEAR_PATTERNS = 0x07
//const CMD_SEND_REPORT_STATUS = 0x08
const CMD_REQUEST_REPORT = 0x09
const CMD_NULL = 0xff

func requestsHandler(conn net.Conn) {
	startTime := time.Now()
	for {
		// if REPORTS_TIME_INTERVAL seconds passed, ask report to the client
		t := time.Now()
		elapsed := t.Sub(startTime).Seconds()
		if elapsed >= REPORTS_TIME_INTERVAL {
			requestReport(conn)
			startTime = time.Now()
		}

		// receive data from the client
		cmd, data, err := receive(conn)
		if err != nil {
			//fmt.Println("[Handler]> Error while receiving the command '",cmd," ",data,"' from '",conn.RemoteAddr(),"'. Error:", err)
			return
		}
		// receive excedeed timeout
		if cmd == CMD_NULL {
			//fmt.Println("[Handler]> Receive timeout exceeded")
			continue
		}

		// Process command
		switch cmd {
		case RECEIVE_ID:
			go handleReceiveID(conn, data)
		case RECEIVE_VERSION:
			go handleReceiveVersion(conn, data)
		case RECEIVE_REPORT:
			go handleReceiveReport(conn, data)
		case RECEIVE_UPDATE:
			// when receive update, reset Reports timer
			startTime = time.Now()
			go handleReceiveUpdate(conn, data)
		case RECEIVE_SUCCESS:
			fmt.Println("[Handler]> '",conn.RemoteAddr(),"': command successfully executed")
		case RECEIVE_ERROR:
			fmt.Println("[Handler]> '",conn.RemoteAddr(),"': error while executing the command '", data,"'")
		default:
			fmt.Printf("[Handler]> Unknown command '%x' received with data: %s\n", cmd, data)
		}
	}
	conn.Close()
}

func handleReceiveID(conn net.Conn, id []byte) {
	fmt.Printf("[Handler]> ' %s ': ID = %d\n", conn.RemoteAddr(), binary.BigEndian.Uint32(id))
}

func handleReceiveVersion(conn net.Conn, version []byte) {
	fmt.Printf("[Handler]> ' %s ': Agent version = %s\n", conn.RemoteAddr(), version)
}

func handleReceiveReport(conn net.Conn, report []byte) {

	// Create output folder
	exec.Command("mkdir", "-p", REPORT_PATH).Run()
	// Generate filename
	filename := strings.Replace(strings.Replace(conn.RemoteAddr().String(), ".", "", -1), ":", "", -1)
	path := REPORT_PATH + filename + ".txt"
	// writing report to file
	err := ioutil.WriteFile(path, report, 0644)
	if check(err, "[Logger]> Error while writing the report file") != 1 {
		fmt.Printf("[Logger]> Report from ' %s ' written to '%s'\n", conn.RemoteAddr(), path)
	}
	// NOTE: when you process the data contained in the report, remember that the process name killed by the Agent is enclosed in bracets
}

func handleReceiveUpdate(conn net.Conn, update []byte) {
	fmt.Printf("[Handler]> ' %s ': %s\n", conn.RemoteAddr(), update)
}

// Request report to the client every 10 seconds
func requestReport(conn net.Conn) {
	time.Sleep(time.Second * 10)
	var cmd byte = CMD_REQUEST_REPORT
	send(conn, cmd, nil)
	fmt.Printf("[Handler]> Report requested to ' %s '\n", conn.RemoteAddr())
}

func requestReboot(conn net.Conn) {
	var cmd byte = CMD_REBOOT
	time.Sleep(10 * time.Second)
	send(conn, cmd, nil)
}
