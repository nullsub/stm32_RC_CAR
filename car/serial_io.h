#ifndef _SERIAL_IO_H_
#define _SERIAL_IO_H_

#include "common.h"

void serial_init();
void debug_msg(char *msg);
void serial_task(void *pvParameters) NORETURN;

#ifdef USE_TERMINAL
#define TERM_CMD_LENGTH		20
#else
#define MAX_COMMAND_LENGTH	20
#define DEBUG_MODE	'0'
#define UPDATE_MODE	'1'
#define REQUEST_MODE	'2'
#endif				// USE_TERMINAL

#endif				// SERIAL_IO_H_
