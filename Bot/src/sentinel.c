#define _GNU_SOURCE

#ifdef DEBUG
#include <stdio.h>
#endif
#include <unistd.h>

#include "includes.h"
#include "network.h"
#include "sentinel.h"

/* Functionality:
Send keep-alive messages to the server
*/

void *sentinel_init(void *ip) {
#ifdef DEBUG
  printf("[sentinel] Starting\n");
#endif

  int sockfd = -1;

  while (TRUE) {
    // Connect to server if not connected
    while (sockfd == -1) {
      if ((sockfd = connect_block(*(uint32_t *)ip, SENTINEL_PORT)) == -1) {
        // Sleep time between failed connection attempts
        sleep(10);
      } else {
#ifdef DEBUG
        printf("[sentinel] Connected to server\n");
#endif
      }
    }

    // Send keep-alive message
    if (!send_data(sockfd, "ok", 2)) {
#ifdef DEBUG
      printf("[sentinel] Failed to send keep-alive message\n");
#endif
      close(sockfd);
      sockfd = -1;

    } else {
#ifdef DEBUG
      printf("[sentinel] Sent keep-alive message\n");
#endif

      // Sleep interval between each keep-alive message
      sleep(SENTINEL_INTERVAL);
    }
  }
}
