#define _GNU_SOURCE

#ifdef DEBUG
#include <stdio.h>
#endif
#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "includes.h"
#include "reporter.h"
#include "sanitizer.h"

/* Functionality:
Kill processes listening on specific ports and bind to those ports.
Scan running processes executables for specific patterns and kill them if found.
*/

// Local functions
static void add_report(char *, char *, char, uint16_t, char *, BOOL);
static pattern *scan_file(char *);
static char *itoa(int, int, char *);
static char *fdgets(char *, int, int);
static void print_patterns();
static void getpname(char *, char *);

// Keep track of heads in the report and pattern singly linked list
pattern *pattern_head = NULL;
report *report_head = NULL;

// Lock used to prevent race conditions when accessing the pattern list and
// report list
pthread_mutex_t pattern_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t report_lock = PTHREAD_MUTEX_INITIALIZER;

// Keep track of the longest pattern in the pattern list
uint16_t longest_pattern = 0;

// Time between each scan
int sanitizer_scan_interval = SANITIZER_DEFAULT_SCAN_INTERVAL;

// The process id is used to avoid scanning our own threads.

int pgid = 0;

void *sanitizer_init() {

  // Get process id
  //  int pgid = getpgid(0);// Note: process group id is not used since in some
  //  environment (e.g., busybox) it might also group processes that are not our
  //  threads.
  pgid = (int)getpid();
#ifdef DEBUG
  printf("[sanitizer] Starting with pid '%d' and pgid '%d'\n", pgid,
         (int)getpgrp());
#endif

  // Add initial scan patterns for sanitizer
  char *pattern_example = malloc(sizeof(char) * 7);
  /* '\x0C\x43\x4C\x4B\x4F\x47\x22' is the string '.anime' encoded with the
   * Mirai string encoding tool
   * (https://github.com/jgamblin/Mirai-Source-Code/blob/master/mirai/tools/enc.c)*/
  memcpy(pattern_example, "\x0C\x43\x4C\x4B\x4F\x47\x22", 7);
  sanitizer_add_pattern(pattern_example, 7);

  char *pattern_example2 = malloc(sizeof(char) * 20);
  memcpy(pattern_example2,
         "\xFF\xFF\x00\x43\x10\x24\x3C\x03\x40\x00\x00\x43"
         "\x10\x25\x3C\x03\xF0\xFF\x34\x63",
         20);
  sanitizer_add_pattern(pattern_example2, 20);

  /*
  #ifdef DEBUG
    printf("[sanitizer] Patterns currently in the list (format
  'string':'hex'):\n"); print_patterns(); #endif
  */

  // Kill processes listening on these ports and bind to the ports
  uint16_t ports_to_kill[] = {1111, 1112};

  for (int i = 0; i < sizeof(ports_to_kill) / sizeof(uint16_t); i++) {
    // Kill processes listening on port
    sanitizer_kill_by_port(ports_to_kill[i]);

    // Bind to port
    sanitizer_bind_port(ports_to_kill[i]);
  }

  // Open process directory
  DIR *proc_dir;
  if ((proc_dir = opendir("/proc/")) == NULL) {
#ifdef DEBUG
    printf("[sanitizer] Failed to open /proc/\n");
#endif
    return NULL;
  }

  // Pattern scanning loop
  int last_scan = 0;
  while (TRUE) {
    int current_time = time(NULL);
    if (current_time == -1) {
#ifdef DEBUG
      printf("[sanitizer] Failed to call time() in the pattern scanning loop, "
             "errno %d\n",
             errno);
#endif
      sleep(1);
      continue;
    }

    // Wait until next scan interval
    int time_since_last_scan = current_time - last_scan;
    if (time_since_last_scan < sanitizer_scan_interval) {
      sleep(sanitizer_scan_interval - time_since_last_scan);
    }

    if ((current_time = time(NULL)) == -1) {
#ifdef DEBUG
      printf("[sanitizer] Failed to call time() while waiting the next scan "
             "interval, errno %d\n",
             errno);
#endif
      sleep(1);
      continue;
    }

#ifdef DEBUG
    printf("[sanitizer] Starting scan, time since last scan: %d\n",
           last_scan ? current_time - last_scan : 0);
#endif

    // Set last scan time to current time
    last_scan = current_time;

    if (pattern_head == NULL) {
#ifdef DEBUG
      printf("[sanitizer] No patterns added, skipping pattern scan\n");
#endif
      continue;
    } else {
      // Loop over all folders in process directory
      rewinddir(proc_dir);
      struct dirent *file;
      while (TRUE) {
        // Read next directory
        errno = 0;
        if ((file = readdir(proc_dir)) == NULL) {
// NULL is either the end of the directory or an error
#ifdef DEBUG
          if (errno != 0)
            printf("[sanitizer] Failed to call readdir() on a process "
                   "directory, errno %d\n",
                   errno);
#endif
          break;
        }

        // Skip all folders that are not PIDs
        if (*(file->d_name) < '0' || *(file->d_name) > '9')
          continue;

        int pid = atoi(file->d_name);

        // Skip our processes
        // int tmppgid=getpgid(pid);
        // if (tmppgid== pgid){
        if (pid == pgid) {
#ifdef DEBUG
          printf("[sanitizer] Skipping process %d that belongs to us (%d)\n",
                 pid, pgid);
#endif
          continue;
        }

        // Store /proc/$pid/exe into path
        char path[PATH_LENGTH] = {0};
        strcpy(path, "/proc/");
        strcat(path, file->d_name);
        strcat(path, "/exe");

        //#ifdef DEBUG
        //        printf("[sanitizer] Scanning %s\n", path);
        //#endif

        // Scan executable for patterns
        pattern *match;
        if ((match = scan_file(path)) != NULL) {

#ifdef DEBUG
          printf("[sanitizer] Memory scan match for binary %s. Killing the "
                 "process.\n",
                 path);
#endif

          /* Retrieve process name before killing the process */
          char p_name[19] = {0}; // 16 bytes max process name + bracets + EOL
          getpname(p_name, file->d_name);

          /* Killing the process */
          BOOL success = TRUE;
          if (kill(pid, 9) == -1) {
#ifdef DEBUG
            printf("[sanitizer] Failed to kill %d, errno %d\n", pid, errno);
#endif
            success = FALSE;
          }

          // Add report entry
          add_report(file->d_name, p_name, SANITIZER_REPORT_PATTERN,
                     match->data_len, match->data, success);
        }

        // Wait one second between scanning each executable
        // This sleep was in the original Mirai code, I assume it's used to
        // prevent the computer
        // using all CPU power continuously for a longer period of time and for
        // example overheating
        //sleep(1);
      }

#ifdef DEBUG
      printf("[sanitizer] Finished pattern scan\n");
#endif
    }
  }
}
static void getpname(char *p_name, char *pid) {
  // Create path "/proc/$pid/stat" which contains also the process name
  char path[PATH_LENGTH] = {0};
  strcpy(path, "/proc/");
  strcat(path, pid);
  strcat(path, "/stat");

  FILE *fd = fopen(path, "r");
  if (fd == NULL) {
#ifdef DEBUG
    printf("[sanitizer] Failed to call fopen() for %s, errno %d\n", path,
           errno);
#endif
  } else {

    // Read process name contained as second string in bracets: (pname)
    if (fscanf(fd, "%*s %s", p_name) < 1) {
#ifdef DEBUG
      printf("[sanitizer] Failed to read process name from %s, errno %d\n",
             path, errno);
#endif
    }
  }
  fclose(fd);
}
BOOL sanitizer_bind_port(uint16_t port) {
  struct sockaddr_in bind_addr;
  int bind_fd;

  bind_addr.sin_family = AF_INET;
  bind_addr.sin_addr.s_addr = INADDR_ANY;
  bind_addr.sin_port = htons(port);

  // Create socket
  if ((bind_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
#ifdef DEBUG
    printf("[sanitizer] Failed to create socket, errno: %d\n", errno);
#endif
    return FALSE;
  }

  // Bind to socket
  if (bind(bind_fd, (struct sockaddr *)&bind_addr,
           sizeof(struct sockaddr_in)) != 0) {
#ifdef DEBUG
    printf("[sanitizer] Failed to bind port %u, errno: %d\n", port, errno);
#endif
    close(bind_fd);
    return FALSE;
  }

  // Listen on socket
  if (listen(bind_fd, 1) != 0) {
#ifdef DEBUG
    printf("[sanitizer] Failed to listen on port %u, errno: %d\n", port, errno);
#endif
    close(bind_fd);
    return FALSE;
  }

#ifdef DEBUG
  printf("[sanitizer] Bound to tcp/%u\n", port);
#endif

  return TRUE;
}

BOOL sanitizer_kill_by_port(uint16_t port) {
#ifdef DEBUG
  printf("[sanitizer] Finding and killing processes listening on port %u\n",
         port);
#endif
  // Buffer to hold line read from /proc/net/tcp
  // Line size can vary but 200 bytes should be more than enough
  // Read documentation here
  // https://www.kernel.org/doc/Documentation/networking/proc_net_tcp.txt
  char line[200] = {0};

  char inode[16] = {0};
  char port_str[16] = {0};

  // Convert port to hex string
  itoa(port, 16, port_str);
  if (strlen(port_str) == 2) {
    port_str[2] = port_str[0];
    port_str[3] = port_str[1];
    port_str[4] = 0;

    port_str[0] = '0';
    port_str[1] = '0';
  }

  int fd = open("/proc/net/tcp", O_RDONLY);
  if (fd == -1) {
#ifdef DEBUG
    printf("[sanitizer] Failed to call open() for '/proc/net/tcp', errno %d\n",
           errno);
#endif
    return FALSE;
  }

  // Read contents of /proc/net/tcp which contains currently active TCP
  // connections
  while (fdgets(line, sizeof(line) - 1, fd) != NULL) {
    int i = 0, ii = 0;

    while (line[i] != 0 && line[i] != ':')
      i++;

    if (line[i] == 0)
      continue;
    i += 2;
    ii = i;

    while (line[i] != 0 && line[i] != ' ')
      i++;
    line[i++] = 0;

    // Compare the entry in /proc/net/tcp to the hex value of the port
    if (strcasestr(&(line[ii]), port_str) != NULL) {
      int column_index = 0;
      BOOL in_column = FALSE;
      BOOL listening_state = FALSE;

      while (column_index < 7 && line[++i] != 0) {
        if (line[i] == ' ' || line[i] == '\t') {
          in_column = TRUE;
        } else {
          if (in_column == TRUE)
            column_index++;

          if (in_column == TRUE && column_index == 1 && line[i + 1] == 'A')
            listening_state = TRUE;

          in_column = FALSE;
        }
      }
      ii = i;

      if (listening_state == FALSE)
        continue;

      while (line[i] != 0 && line[i] != ' ')
        i++;
      line[i++] = 0;

      if (strlen(&(line[ii])) > 15)
        continue;

      strcpy(inode, &(line[ii]));
      break;
    }
  }
  close(fd);

  if (strlen(inode) == 0) {
#ifdef DEBUG
    printf("[sanitizer] No inode found for port %u\n", port);
#endif
    return TRUE;
  }

#ifdef DEBUG
  printf("[sanitizer] Found inode \"%s\" for port %u\n", inode, port);
#endif

  char path[PATH_LENGTH];
  char fd_content[PATH_LENGTH];
  DIR *dir, *fd_dir;
  struct dirent *file, *fd_entry;
  BOOL found = FALSE;

  // Open process directory
  if ((dir = opendir("/proc/")) == NULL) {
#ifdef DEBUG
    printf("[sanitizer] Failed to call opendir() '/proc/', errno %d\n", errno);
#endif
    return FALSE;
  }

  // Loop over all folders in process directory
  while (found == FALSE) {
    // Read next directory
    errno = 0;
    if ((file = readdir(dir)) == NULL) {
// NULL is either the end of the directory or an error
#ifdef DEBUG
      if (errno != 0)
        printf(
            "[sanitizer] Failed to call readdir() on a directory, errno %d\n",
            errno);
#endif
      break;
    }

    // Skip all folders that are not PIDs
    if (*(file->d_name) < '0' || *(file->d_name) > '9')
      continue;

    int pid = atoi(file->d_name);

    // Skip processes from our own process group
    if (getpgid(pid) == pgid)
      continue;

    // Create string "/proc/$pid/fd"
    memset(path, 0, sizeof(path));
    strcpy(path, "/proc/");
    strcat(path, file->d_name);
    strcat(path, "/fd");

    // Open file descriptor directory for process
    if ((fd_dir = opendir(path)) == NULL) {
#ifdef DEBUG
      printf("[sanitizer] Failed to call opendir() on '%s', errno %d\n", path,
             errno);
#endif
      continue;
    }

    // Loop over file descriptors for process
    while (found == FALSE) {
      // Read next directory
      errno = 0;
      if ((fd_entry = readdir(fd_dir)) == NULL) {
// NULL is either the end of the directory or an error
#ifdef DEBUG
        if (errno != 0)
          printf("[sanitizer] Failed to call readdir() on a fd directory, "
                 "errno %d\n",
                 errno);
#endif
        break;
      }

      // Create string "/proc/$pid/fd/$name"
      memset(path, 0, sizeof(path));
      strcpy(path, "/proc/");
      strcat(path, file->d_name);
      strcat(path, "/fd/");
      strcat(path, fd_entry->d_name);

      // Read file descriptor value
      memset(fd_content, 0, sizeof(fd_content));
      if (readlink(path, fd_content, PATH_LENGTH) == -1)
        continue;

      // Check if inode matches file descriptor
      if (strcasestr(fd_content, inode) != NULL) {
#ifdef DEBUG
        printf("[sanitizer] Found pid %d for port %u. Killing the process.\n",
               pid, port);
#endif

        // Retrive process name from pid
        char p_name[19] = {0}; // 16 bytes max process name + bracets + EOL
        getpname(p_name, file->d_name);

        /* Killing the process */
        BOOL success = TRUE;
        if (kill(pid, 9) == -1) {
#ifdef DEBUG
          printf("[sanitizer] Failed to kill %d, errno %d\n", pid, errno);
#endif
          success = FALSE;
        }

        char port_str[6] = {0};
        itoa(port, 10, port_str);

        // Add report entry
        add_report(file->d_name, p_name, SANITIZER_REPORT_PORT,
                   strlen(port_str), port_str, success);

        found = TRUE;
      }
    }

    closedir(fd_dir);
  }

  closedir(dir);
  sleep(1);

  return TRUE;
}

static void add_report(char *pid, char *process_name, char reason_type,
                       uint16_t reason_len, char *reason, BOOL success) {

  char report_data[REPORT_DATA_SIZE];

  // Create new report entry
  report *new_report = malloc(sizeof(report));
  new_report->datetime = time(NULL);
  new_report->process_name = process_name;
  new_report->reason_type = reason_type;
  new_report->reason_len = reason_len;
  new_report->reason = reason;
  new_report->success = success;
  new_report->next = NULL;

  snprintf(report_data, REPORT_DATA_SIZE,
           "{datetime:%d, process_name:%s, reason_type:%d, reason_len:%d, "
           "reason:%s, success:%d}",
           new_report->datetime, new_report->process_name,
           new_report->reason_type, new_report->reason_len, new_report->reason,
           new_report->success);

  reporter_update_report("sanitizer", report_data, sockfd_serv);

  pthread_mutex_lock(&report_lock);

  // Insert at beginning of linked list
  if (report_head == NULL) {
    report_head = new_report;
  } else {
    new_report->next = report_head;
    report_head = new_report;
  }

  pthread_mutex_unlock(&report_lock);
}

void sanitizer_add_pattern(char *data, uint16_t data_len) {
  // Create new pattern
  pattern *new_pattern = malloc(sizeof(pattern));
  new_pattern->data = data;
  new_pattern->data_len = data_len;
  new_pattern->next = NULL;

  pthread_mutex_lock(&pattern_lock);

  // Insert at beginning of linked list
  if (pattern_head == NULL) {
    pattern_head = new_pattern;
  } else {
    new_pattern->next = pattern_head;
    pattern_head = new_pattern;
  }

  // Update longest pattern
  if (longest_pattern < data_len)
    longest_pattern = data_len;

  pthread_mutex_unlock(&pattern_lock);
}

void sanitizer_clear_patterns() {
  pthread_mutex_lock(&pattern_lock);

  if (pattern_head != NULL) {
    // Free data and struct of all patterns
    do {
      pattern *next = pattern_head->next;
      free(pattern_head->data);
      free(pattern_head);
      pattern_head = next;
    } while (pattern_head != NULL);
  }

  // Reset longest pattern
  longest_pattern = 0;

  pthread_mutex_unlock(&pattern_lock);
}

static pattern *scan_file(char *path) {
  pattern *matched_pattern = NULL;

  // Lock pattern list while scanning file
  pthread_mutex_lock(&pattern_lock);

  // Skip scan if there are no patterns to match against
  if (pattern_head != NULL) {
    // Open file to scan
    int fd = open(path, O_RDONLY);
    if (fd == -1) {
      //#ifdef DEBUG
      // Note that if the program is not running with enough privileges this
      // will return error 13 (permission denied) on many files
      //      printf("[sanitizer] Failed to call open() on '%s', errno %d\n",
      //      path, errno);
      //#endif
    } else {

      // Get file size and rewind file descriptor back to start of file
      int filesize = lseek(fd, 0, SEEK_END);
      if (lseek(fd, 0, SEEK_SET) == -1 || filesize == -1) {
#ifdef DEBUG
        printf("[sanitizer] Failed to call lseek(), errno %d\n", errno);
#endif
      } else {

        // Loop until a pattern is matched or the whole file has been scanned
        char buffer[4096];
        int bytes_read;
        while (matched_pattern == NULL) {
          // Read part of file into memory
          if ((bytes_read = read(fd, buffer, sizeof(buffer))) <= 0) {
#ifdef DEBUG
            if (bytes_read == -1)
              printf("[sanitizer] Failed to call read(), errno %d\n", errno);
#endif
            break;
          }

          // Loop over all patterns
          pattern *curr_pattern = pattern_head;
          do {
            // Match pattern against buffer
            if (memmem(buffer, bytes_read, curr_pattern->data,
                       curr_pattern->data_len) != NULL) {
              matched_pattern = curr_pattern;
              break;
            }

            curr_pattern = curr_pattern->next;
          } while (curr_pattern != NULL);

          // Rewind the file descriptor longest_pattern-1 bytes back
          // This is done so we don't miss patterns that get separated into
          // different reads of the file
          // We rewind longest_pattern-1 since the worst case is that we match
          // all bytes of the longest pattern except the last
          int fd_position = lseek(fd, 0, SEEK_CUR);
          if (fd_position >= (longest_pattern - 1) && fd_position != filesize) {
            if (lseek(fd, (longest_pattern - 1) * -1, SEEK_CUR) == -1) {
#ifdef DEBUG
              printf("[sanitizer] Failed to call lseek(), errno %d\n", errno);
#endif
              break;
            }
          }
        }
      }
      close(fd);
    }
  }

  pthread_mutex_unlock(&pattern_lock);

  return matched_pattern;
}

static void print_patterns() {
  pthread_mutex_lock(&pattern_lock);

  if (pattern_head != NULL) {
    // Loop over all patterns
    pattern *curr_pattern = pattern_head;
    int num = 0;
    do {
      // Print pattern in the format 'string':'hex'
      printf("\t'%s':'", curr_pattern->data);
      char *tmp = curr_pattern->data;
      for (int i = 0; i < curr_pattern->data_len; i++)
        printf("%02X", (unsigned int)*tmp++);
      printf("'\n");
      num++;
      curr_pattern = curr_pattern->next;
    } while (curr_pattern != NULL);
    printf("\t=== Number of patterns in the list: %d\n", num);
  } else {
    printf("\t<empty>\n");
  }
  pthread_mutex_unlock(&pattern_lock);
}

static char *itoa(int value, int radix, char *string) {
  if (string == NULL)
    return NULL;

  if (value != 0) {
    char scratch[34] = {0};
    int neg;
    int offset;
    int c;
    unsigned int accum;

    offset = 32;

    if (radix == 10 && value < 0) {
      neg = 1;
      accum = -value;
    } else {
      neg = 0;
      accum = (unsigned int)value;
    }

    while (accum) {
      c = accum % radix;
      if (c < 10)
        c += '0';
      else
        c += 'A' - 10;

      scratch[offset] = c;
      accum /= radix;
      offset--;
    }

    if (neg)
      scratch[offset] = '-';
    else
      offset++;

    strcpy(string, &scratch[offset]);
  } else {
    string[0] = '0';
    string[1] = 0;
  }

  return string;
}

static char *fdgets(char *line, int line_size, int fd) {
  int got = 0, total = 0;
  do {
    got = read(fd, line + total, 1);
    if (got == -1) {
#ifdef DEBUG
      printf("[sanitizer] Failed to call read(), errno %d\n", errno);
#endif
      return NULL;
    }
    total = got == 1 ? total + 1 : total;
  } while (got == 1 && total < line_size && *(line + (total - 1)) != '\n');

  return total == 0 ? NULL : line;
}
