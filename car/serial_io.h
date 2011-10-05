#ifndef _SERIAL_IO_H_
#define _SERIAL_IO_H_

void debug_msg(char *msg);

#ifdef USE_TERMINAL
#define TERM_CMD_LENGTH		50
#else
#define MAX_COMMAND_LENGTH	254
#define DEBUG_MODE	0
#define UPDATE_MODE	1
#define REQUEST_MODE	2
#endif				// USE_TERMINAL

#endif				// SERIAL_IO_H_
