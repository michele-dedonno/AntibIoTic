#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "includes.h"
#include "network.h"
#include "reporter.h"

/* Functionality:
   Generates a JSON summary of the machine's state.
*/

void reporter_create_report() {
  char filename[] = REPORT_PATH;
#ifdef DEBUG
  printf("[reporter] Creating report file: '%s'\n", filename);
#endif

  // opening the file, discarding a potential previous one
  FILE *pFile = fopen(filename, "w");

  if (pFile != NULL) {
    fclose(pFile);
  } else {
#ifdef DEBUG
    printf("[reporter] Could not create report file\n");
#endif
  }
}

void reporter_append_report(char *module, char *data) {
  char report_path[] = REPORT_PATH;

  FILE *pFile = fopen(report_path, "a");

  flockfile(pFile);

  fprintf(pFile, "{module:%s, timestamp:%lu, data:%s}", module,
          (unsigned long)time(NULL), data);
  fflush(pFile);

  funlockfile(pFile);
  fclose(pFile);
#ifdef DEBUG
  printf("[reporter] Successfully appended update to report\n");
#endif
}

void reporter_update_report(char *module, char *data, int sock) {
#ifdef DEBUG
  printf("[stub] Updating report\n");
#endif
  reporter_append_report(module, data);

  char buf[1000];
  memset(buf, '\0', sizeof(buf));

  snprintf(buf, sizeof(buf), "{module:%s, timestamp:%lu, data:%s}", module,
           (unsigned long)time(NULL), data);

  command *cmd = (command *)malloc(sizeof(command));
  memset(cmd, '\0', sizeof(command));
  cmd->type = CMD_SEND_UPDATE;
  cmd->data_len = sizeof(buf);
  cmd->data = buf;

  send_command(sock, cmd);

  free(cmd);
}

void reporter_send_report(int sockfd) {
  struct stat stat_report;
  char filename[] = REPORT_PATH;

  FILE *pFile = fopen(filename, "r");

  if (pFile == NULL) {
#ifdef DEBUG
    printf("[reporter] error opening report file\n");
#endif
    return;
  }

  flockfile(pFile);

  if (fstat(fileno(pFile), &stat_report) < 0) {
#ifdef DEBUG
    printf("[reporter] error opening file stats\n");
#endif
    funlockfile(pFile);
    fclose(pFile);
    return;
  }

  command *cmd = (command *)malloc(sizeof(command));
  memset(cmd, '\0', sizeof(*cmd));
  cmd->type = CMD_SEND_REPORT;
  cmd->data_len = (uint16_t)stat_report.st_size;
  cmd->data = malloc(sizeof(char) * cmd->data_len);
  memset(cmd->data, '\0', sizeof(char) * cmd->data_len);

  if (fread(cmd->data, cmd->data_len, 1, pFile) != 1) {
#ifdef DEBUG
    printf("[reporter] error reading file\n");
#endif
    funlockfile(pFile);
    free(cmd->data);
    free(cmd);
    fclose(pFile);
    return;
  }

  // cleaning the file
  freopen(REPORT_PATH, "w", pFile);
  funlockfile(pFile);
  fclose(pFile);

  send_command(sockfd, cmd);
  free(cmd->data);
  free(cmd);

#ifdef DEBUG
  printf("[reporter] Successfully sent report\n");
#endif
}
