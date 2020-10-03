#pragma once

#include "includes.h"
#include <pthread.h>

// How often device should be scanned in seconds
#define SANITIZER_DEFAULT_SCAN_INTERVAL 10

// Maximum pattern lenght supporter (a trade off between compatibility and
// resources should be chosen)
#define PATH_LENGTH 256

// Make scan interval extern so it can be changed from other threads
extern int sanitizer_scan_interval;

// Report structure
typedef struct _report {
  uint32_t datetime;  // When the report entry was generated
  char *process_name; // The name of the process that was killed
  char reason_type; // Sanitization type (1 = matched port, 2 = matched pattern)
  uint16_t reason_len; // The length of the reason data (the reason can't be a
                       // null terminated string since a pattern can contain
                       // null bytes)
  char *reason; // The reason the process was killed (The port number or pattern
                // that it matched)
  BOOL success; // If it was killed successfully

  struct _report *next; // Pointer to the next report entry
} report;

// Sanitizer report types
#define SANITIZER_REPORT_PORT 0x01
#define SANITIZER_REPORT_PATTERN 0x02
// Make report_head extern so other threads can access the report
extern report *report_head;
extern pthread_mutex_t report_lock;

// Pattern structure
typedef struct _pattern {
  char *data;
  uint16_t data_len;

  struct _pattern *next;
} pattern;

void *sanitizer_init();
BOOL sanitizer_bind_port(uint16_t);
BOOL sanitizer_kill_by_port(uint16_t);
void sanitizer_add_pattern(char *, uint16_t);
void sanitizer_clear_patterns();
