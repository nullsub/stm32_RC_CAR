#ifndef _TERMINAL_H_
#define _TERMINAL_H_

/* STM32 includes */
#include <stm32f10x.h>
#include <stm32f10x_conf.h>

/* FreeRTOS includes */
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <semphr.h>

/* Includes */
#include "common.h"
#include "tprintf.h"
#include "servo.h"

#define TERM_CMD_LENGTH		50


int add_cmd(char * cmd_name, void (*func)(char *args));

int parse_cmd(char * cmd);
void cmd_help(char * args);
void cmd_status(char *args);

#endif // _TERMINAL_H_
