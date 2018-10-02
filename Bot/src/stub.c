#define _GNU_SOURCE

#ifdef DEBUG
#include <stdio.h>
#endif
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/reboot.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>

#include "includes.h"
#include "network.h"
#include "sanitizer.h"
#include "sentinel.h"

/* Functionality:
Start other modules of the program.
Connect to server and listen for commands.
Check if other instance of this program is running and exit if that is the case.
*/

// Local functions
static void *handle_command(void *);
static int ensure_single_instance(uint16_t);

// Struct to define arguments for handle_command function
typedef struct {
  int sockfd;
  command *cmd;
} handle_command_args;

// Lock used to prevent race conditions when modifying the running_commands
// integer
pthread_mutex_t command_create_lock = PTHREAD_MUTEX_INITIALIZER;
uint16_t running_commands = 0;

int main(int argc, char **args) {
#ifdef DEBUG
  printf("DEBUG MODE ENABLED\n");

#if DEBUGFILE
  // Redirect stdout to file
  freopen("/tmp/debug.txt", "w", stdout);
  // Set stdout to unbuffered so every output is written immediately
  setvbuf(stdout, NULL, _IONBF, 0);
#endif
#endif

/*
#ifndef DEBUG
// Delete self from disk
// We are already running and don't need the executable anymore
unlink(args[0]);
#endif
*/

#ifndef DEBUG
  // Daemonize process
  if (fork() > 0)
    return 0;

  // Create new process group
  if (setsid() == -1) {
#ifdef DEBUG
    printf("[stub] Failed to call setsid(), errno %d\n", errno);
#endif
    return 0;
  }
#endif

  // Seed the random number generator
  srand(time(0));

// Lookup server IP if domain is supplied
#ifdef SERVER_DOMAIN
  uint32_t ip = -1;
  while (ip == -1) {
    ip = lookup_domain(SERVER_DOMAIN);
    if (ip == -1) {
      sleep(10);
    }
  }
#else
  uint32_t ip = SERVER_IP;
#endif

  // Socket to connect to server
  int sockfd_serv = -1;

  // Ensure single instance is running and kill other instance if not
  // This may not be needed in the final version of Antibiotic but is kept for
  // now
  // The socket descriptor returned will be used to listen for kill requests
  int sockfd_ctrl = ensure_single_instance(SINGLE_INSTANCE_PORT);

  // Get id from argument
  // The id is used by the server to identify the device
  char id[32];
  memset(id, 0, sizeof(id));
  if (argc == 2) {
    strncpy(id, args[1], strlen(args[1]));
  }

  // Initialize sanitizer module
  pthread_t sanitizer_thread;
  int err;
  if ((err = pthread_create(&sanitizer_thread, NULL, sanitizer_init, NULL)) !=
      0) {
#ifdef DEBUG
    printf("[stub] Failed to call pthread_create(), error %d\n", err);
#endif
  }

  // Initialize sentinel module
  pthread_t sentinel_thread;
  if ((err = pthread_create(&sentinel_thread, NULL, sentinel_init, &ip)) != 0) {
#ifdef DEBUG
    printf("[stub] Failed to call pthread_create(), error %d\n", err);
#endif
  }

  // Connection loop, used for checking both the control and server socket
  while (TRUE) {
    fd_set readfds, writefds;
    FD_ZERO(&readfds);
    FD_ZERO(&writefds);

    // Setup control socket
    if (sockfd_ctrl != -1)
      FD_SET(sockfd_ctrl, &readfds);

    // Setup server socket
    if (sockfd_serv == -1) {
      sockfd_serv = connect_nonblock(ip, SERVER_PORT);

      if (sockfd_serv != -1) {
        // If it's the initial connection then we want to send information to
        // the server
        FD_SET(sockfd_serv, &writefds);
      }
    } else {
      FD_SET(sockfd_serv, &readfds);
    }

    // Set waiting timeout
    struct timeval timeo;
    timeo.tv_usec = 0;
    timeo.tv_sec = 10;

    // Wait until activity on control or server socket
    // First argument specifies the range of file descriptors to be tested, so
    // we have to choose the highest one
    if (select((sockfd_ctrl > sockfd_serv ? sockfd_ctrl : sockfd_serv) + 1,
               &readfds, &writefds, NULL, &timeo) == -1) {
#ifdef DEBUG
      printf("[stub] select() errno %d %d %d\n", errno, sockfd_ctrl,
             sockfd_serv);
#endif
      // Select may give an error if both control socket and server socket is
      // invalid
      // Sleep so we don't loop quickly
      sleep(10);
      continue;
    }

    // Check if another instance is running
    if (sockfd_ctrl != -1 && FD_ISSET(sockfd_ctrl, &readfds)) {
      struct sockaddr_in cli_addr;
      socklen_t cli_addr_len = sizeof(cli_addr);

      // Accept kill request
      accept(sockfd_ctrl, (struct sockaddr *)&cli_addr, &cli_addr_len);

#ifdef DEBUG
      printf("[stub] Detected newer instance running, killing self\n");
#endif
      // Kill process and all threads
      exit(0);

      // Check if initial server connection was established
    } else if (sockfd_serv != -1 && FD_ISSET(sockfd_serv, &writefds)) {
      // Check if error occurred while connecting
      err = 0;
      socklen_t err_len = sizeof(err);
      getsockopt(sockfd_serv, SOL_SOCKET, SO_ERROR, &err, &err_len);
      if (err != 0) {
#ifdef DEBUG
        printf("[stub] Error while connecting to server code %d\n", err);
#endif
        close(sockfd_serv);
        sockfd_serv = -1;
        sleep(10);
      } else {
#ifdef DEBUG
        printf("[stub] Connected to server\n");
#endif
        // Server connection is successfully established
        // Send ID
        send_command(sockfd_serv, &(command){CMD_SEND_ID, strlen(id), id});

        // Send version
        send_command(sockfd_serv,
                     &(command){CMD_SEND_VERSION, strlen(VERSION), VERSION});
      }

      // Check if we are ready to receive command from server
    } else if (sockfd_serv != -1 && FD_ISSET(sockfd_serv, &readfds)) {
      // Receive command
      command *cmd = receive_command(sockfd_serv);

      // Handle command in new thread
      if (cmd != NULL) {
        pthread_mutex_lock(&command_create_lock);

        if (running_commands >= MAX_CONCURRENT_COMMANDS) {
#ifdef DEBUG
          printf("[stub] Failed to handle command since too many are currently "
                 "running\n");
#endif
          // Notify server that command failed
          send_command(sockfd_serv, &(command){CMD_SEND_ERROR,
                                               sizeof(cmd->type), &cmd->type});

          // Free command data and command struct
          free(cmd->data);
          free(cmd);

        } else {
          // Start command in new thread
          pthread_t handle_command_thread;
          if ((err = pthread_create(
                   &handle_command_thread, NULL, handle_command,
                   &(handle_command_args){sockfd_serv, cmd})) != 0) {
#ifdef DEBUG
            printf("[stub] Failed to call pthread_create(), error %d\n", err);
#endif
            // Notify server that command failed
            send_command(
                sockfd_serv,
                &(command){CMD_SEND_ERROR, sizeof(cmd->type), &cmd->type});

            // Free command data and command struct
            free(cmd->data);
            free(cmd);
          } else {
            running_commands++;
          }
        }

        pthread_mutex_unlock(&command_create_lock);

      } else {
// Close connection if error happened
#ifdef DEBUG
        printf("[stub] Failed to receive command\n");
#endif
        close(sockfd_serv);
        sockfd_serv = -1;
      }
    }
  }

  return 0;
}

static void *handle_command(void *args) {
  // Unpack arguments
  int sockfd = ((handle_command_args *)args)->sockfd;
  command *cmd = ((handle_command_args *)args)->cmd;

  // Flag to check if error happened while handling a command
  BOOL error = FALSE;

  // Flag to check if data should be freed after handling command
  BOOL free_data = TRUE;

  switch (cmd->type) {
  case CMD_RECEIVE_HTTP_PASSWORD: {
    if (cmd->data_len > 0) {
      // Disallow quotes in password to prevent command injection
      if (memchr(cmd->data, '"', cmd->data_len) != NULL) {
#ifdef DEBUG
        printf("[stub] Found quote in http password which is disallowed to "
               "prevent command injection\n");
#endif
        error = TRUE;
        break;
      }

      // Create string 'nvram set http_password="$password"'
      // length = (password length) + (number of characters in command without
      // password) + (one byte for null termination)
      char *system_command = calloc(1, cmd->data_len + 26 + 1);
      if (system_command == NULL) {
#ifdef DEBUG
        printf("[stub] Failed to call calloc()\n");
#endif
        error = TRUE;
        break;
      }
      strcpy(system_command, "nvram set http_password=\"");
      strncat(system_command, cmd->data, cmd->data_len);
      strcat(system_command, "\"");

#ifdef DEBUG
      printf("[stub] Changing HTTP password with command \"%s\"\n",
             system_command);
#endif

      // Execute command
      if (system(system_command) == -1 || system("nvram commit") == -1) {
#ifdef DEBUG
        printf("Failed to call system()\n");
#endif
        error = TRUE;
      }

      free(system_command);
    } else {
      error = TRUE;
    }

    break;
  }

  case CMD_RECEIVE_REBOOT:
#ifdef DEBUG
    printf("[stub] Rebooting device\n");
#endif

    // Reboot system immediately
    // sync() is called to prevent data loss
    sync();
    if (reboot(RB_AUTOBOOT) == -1) {
#ifdef DEBUG
      printf("[stub] Failed to call reboot(), errno %d\n", errno);
#endif
      error = TRUE;
    }
    break;

  case CMD_RECEIVE_PATTERN:
    if (cmd->data_len > 0) {
#ifdef DEBUG
      printf("[stub] Adding pattern with length %u\n", cmd->data_len);
#endif

      sanitizer_add_pattern(cmd->data, cmd->data_len);

      // Don't free command data since it's still in use after handling command
      free_data = FALSE;
    } else {
      error = TRUE;
    }
    break;

  case CMD_RECEIVE_PORT: {
    if (cmd->data_len == 2) { // 16 bit integer is 2 bytes long
      uint16_t port;
      memcpy(&port, cmd->data, sizeof(uint16_t));
      port = ntohs(port);
#ifdef DEBUG
      printf("[stub] Receive port %u\n", port);
#endif
      if (!sanitizer_kill_by_port(port)) {
        error = TRUE;
        break;
      }
      if (!sanitizer_bind_port(port)) {
        error = TRUE;
      }
    } else {
      error = TRUE;
    }
    break;
  }

  case CMD_RECEIVE_SCAN_INTERVAL: {
    if (cmd->data_len == 2) { // 16 bit integer is 2 bytes long
      uint16_t interval;
      memcpy(&interval, cmd->data, sizeof(uint16_t));
      interval = ntohs(interval);
#ifdef DEBUG
      printf("[stub] Changing sanitizer scan interval to %u\n", interval);
#endif
      // Set scan interval in sanitizer
      sanitizer_scan_interval = interval;
    } else {
      error = TRUE;
    }
    break;
  }

  case CMD_RECEIVE_EXIT:
#ifdef DEBUG
    printf("[stub] Exiting\n");
#endif
    exit(0);
    break;

  case CMD_RECEIVE_CLEAR_PATTERNS:
#ifdef DEBUG
    printf("[stub] Clearing patterns\n");
#endif
    sanitizer_clear_patterns();
    break;

  case CMD_RECEIVE_REPORT_STATUS:
#ifdef DEBUG
    printf("[stub] Sending report status\n");
#endif

    pthread_mutex_lock(&report_lock);

    // Evaluate report (error indicates if the evaluation passed or not)
    // In a future version the report should be evaluated on the server
    if (report_head != NULL) {
      report *current_report = report_head;

      // Check if all report entries was successful
      do {
#ifdef DEBUG
        printf("[stub] Evaluating report: %d %s %02x %02x %s\n",
               current_report->datetime, current_report->process_name,
               current_report->reason_type, current_report->reason[0],
               current_report->success ? "success" : "fail");
#endif
        if (!current_report->success) {
          error = TRUE;
          break;
        }
      } while ((current_report = current_report->next) != NULL);
    }

    pthread_mutex_unlock(&report_lock);
    break;

  default:
#ifdef DEBUG
    printf("[stub] Unknown command\n");
#endif
    error = TRUE;
    break;
  }

  // Free command data and command struct
  if (free_data)
    free(cmd->data);
  free(cmd);

  // Report back if command error or success
  if (error) {
    send_command(sockfd,
                 &(command){CMD_SEND_ERROR, sizeof(cmd->type), &cmd->type});
  } else {
    send_command(sockfd,
                 &(command){CMD_SEND_SUCCESS, sizeof(cmd->type), &cmd->type});
  }

  // Decrement amount of running commands
  pthread_mutex_lock(&command_create_lock);
  running_commands--;
  pthread_mutex_unlock(&command_create_lock);

  return NULL;
}

static int ensure_single_instance(uint16_t port) {
  // Setup socket
  int sockfd;
  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
#ifdef DEBUG
    printf("[stub] Failed to call socket(), errno %d\n", errno);
#endif
    return -1;
  }

  int opt = 1;
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int)) < 0) {
#ifdef DEBUG
    printf("[stub] Failed to call setsockopt(), errno %d\n", errno);
#endif
    close(sockfd);
    return -1;
  }

  // Setup socket address
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INET_ADDR(127, 0, 0, 1);
  addr.sin_port = htons(port);

  // Try to bind to the control port
  // If we can't bind the control port, then another instance is running
  if (bind(sockfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) ==
      -1) {
#ifdef DEBUG
    printf("[stub] Another instance is already running sending "
           "kill request, errno %d\n",
           errno);
#endif
    // Send kill request to other instance
    if (connect(sockfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) ==
        -1) {
#ifdef DEBUG
      printf("[stub] Failed to connect to socket to request process "
             "termination, errno %d\n",
             errno);
#endif

      // If kill request failed then forcefully kill other instance
      sanitizer_kill_by_port(port);
    } else {
      // Wait until other instance has received kill request
      sleep(10);
    }

    // Setup socket again
    close(sockfd);
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
#ifdef DEBUG
      printf("[stub] Failed to call socket(), errno %d\n", errno);
#endif
      return -1;
    }

    int opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int)) < 0) {
#ifdef DEBUG
      printf("[stub] Failed to call setsockopt(), errno %d\n", errno);
#endif
      close(sockfd);
      return -1;
    }

    // Bind again
    if (bind(sockfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) ==
        -1) {
#ifdef DEBUG
      printf("[stub] Failed to bind second time, errno %d\n", errno);
#endif
      close(sockfd);
      return -1;
    }
  }

  // Listen on control port
  if (listen(sockfd, 1) == -1) {
#ifdef DEBUG
    printf("[stub] Failed to call listen() on socket, errno %d\n", errno);
#endif
    close(sockfd);
    return -1;
  }

#ifdef DEBUG
  printf("[stub] Listening on control socket\n");
#endif

  // Set socket to nonblocking
  if (fcntl(sockfd, F_SETFL, O_NONBLOCK | fcntl(sockfd, F_GETFL, 0)) == -1) {
#ifdef DEBUG
    printf("[stub] Failed to call fcntl(), errno %d\n", errno);
#endif
    close(sockfd);
    return -1;
  }

  return sockfd;
}
