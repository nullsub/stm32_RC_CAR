#ifndef _SERIAL_IO_H_
#define _SERIAL_IO_H_

void debug_msg(char *msg);
void update_var(char *name, int val);

#ifdef USE_TERMINAL
#define TERM_CMD_LENGTH		50

int add_cmd(char *cmd_name, void (*func) (char *args));
int parse_cmd(char *cmd);
void cmd_help(char *args);
void cmd_status(char *args);
void cmd_servo_cal(char *args);
void cmd_servo(char *args);
#else
#define MAX_COMMAND_LENGTH	254

#define DEBUG_MODE	0
#define UPDATE_MODE	1
#define REQUEST_MODE	2

#endif				// USE_TERMINAL

#endif				// SERIAL_IO_H_
