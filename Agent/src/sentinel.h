#pragma once

#include "includes.h"

// Port on server that sentinel connects to
#define SENTINEL_PORT 4321

// Interval between each keep-alive message in seconds
#define SENTINEL_INTERVAL 15 

void *sentinel_init();
