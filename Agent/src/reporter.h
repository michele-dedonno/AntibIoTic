#ifndef __REPORTER_H
#define __REPORTER_H

#include "includes.h"
#include <stdio.h>

// Interval between each report generation
#define REPORT_INTERVAL 10

void reporter_init(char *);
void reporter_send_report(int sock);
void reporter_write_report();
void reporter_create_report();
void reporter_append_report(char *module, char *data);
void reporter_update_report(char *module, char *data, int sock);

#endif
