#pragma once

#include <stdint.h>

#define FALSE 0
#define TRUE 1
typedef char BOOL;

#define INET_ADDR(o1, o2, o3, o4)                                              \
  (htonl((o1 << 24) | (o2 << 16) | (o3 << 8) | (o4 << 0)))

// Program version
// This is sent to the server on initial connection
#define VERSION "1.0"

// Server information
// If SERVER_DOMAIN is defined then the IP will be resolved by a DNS lookup,
// otherwise the IP defined in SERVER_IP will be used
//#define SERVER_DOMAIN ""
#define SERVER_IP INET_ADDR(127, 0, 0, 1)
#define SERVER_PORT 1234

// Port used to check if another instance is running
#define SINGLE_INSTANCE_PORT 47861

// Maximum number of command handling threads
// The current number is arbitrarily chosen
#define MAX_CONCURRENT_COMMANDS 50

// Biggest command data length allowed
#define CMD_MAX_LENGTH 1000

// Commands types
#define CMD_SEND_ID 0x01
#define CMD_SEND_VERSION 0x02
#define CMD_SEND_SUCCESS 0x88
#define CMD_SEND_ERROR 0x99

#define CMD_RECEIVE_HTTP_PASSWORD 0x01
#define CMD_RECEIVE_REBOOT 0x02
#define CMD_RECEIVE_PATTERN 0x03
#define CMD_RECEIVE_PORT 0x04
#define CMD_RECEIVE_SCAN_INTERVAL 0x05
#define CMD_RECEIVE_EXIT 0x06
#define CMD_RECEIVE_CLEAR_PATTERNS 0x07
#define CMD_RECEIVE_REPORT_STATUS 0x08

// Command structure
typedef struct {
  char type;
  uint16_t data_len;
  char *data;
} command;
