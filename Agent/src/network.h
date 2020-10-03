#pragma once

#include "includes.h"

BOOL send_data(int, const void *, int);
BOOL send_file(int sockfd, char file_path[]);
BOOL send_command(int, command *);
command *receive_command(int);
int connect_nonblock(uint32_t, uint16_t);
int connect_block(uint32_t, uint16_t);
uint32_t lookup_domain(char *);
