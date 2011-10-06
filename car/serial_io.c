#include "serial_io.h"
#include "car_status.h"
#include "common.h"
#include "tprintf.h"

#include "FreeRTOS.h"
#include "queue.h"

#include <string.h>

#ifdef USE_TERMINAL
#include "servo.h"
#endif

extern xQueueHandle uart_receive_queue;

static int get_word(char *buffer, char *source, int length);

#ifndef USE_TERMINAL
static void handle_package(char *command, char mode);
static void send_all_vals();
static void send_package(char *command, char mode);

#define ARG_LENGTH 7 //length of arg

void serial_task(void *pvParameters)	//remote_command_task
{
	char command[MAX_COMMAND_LENGTH];
	char length = 0;
	char mode = 0;
	char ch;

	status_init();

	for (;;) {
		/* get length and mode */
		xQueueReceive(uart_receive_queue, &length, portMAX_DELAY);
		xQueueReceive(uart_receive_queue, &mode, portMAX_DELAY);	
		for (int i = 0; i < (unsigned int) length; i++) {
			xQueueReceive(uart_receive_queue, &ch, portMAX_DELAY);	
			command[i] = ch;
		}
		command[(int)length] = 0x00;
		handle_package(command, mode);
	}
}

void handle_package(char *command, char mode)
{
	switch (mode) {
	case DEBUG_MODE:
		debug_msg("debug mode enabled");
		break;
	case REQUEST_MODE:	// remote app requests vars.
		send_all_vals();
		break;
	case UPDATE_MODE:{	//Remote app sends updated vars
			int string_i = 0;
			int length = strlen(command);
			char index[ARG_LENGTH] ;
			char val[ARG_LENGTH];
			while (string_i < length) {
				string_i += get_word(index, (command+string_i), ARG_LENGTH);
				string_i += get_word(val, (command + string_i), ARG_LENGTH);
				status_update_var(atoi(index), atoi(val));
			}
		}
		break;
	default: 
		debug_msg("unknown mode!");
		break;
	}
	return;
}

void send_all_vals()
{
	char command[MAX_COMMAND_LENGTH];
	status_get_var_str(command);
	debug_msg("send_all_vals: package is");
	debug_msg(command);
	send_package(command, UPDATE_MODE);
}

void send_package(char *command, char mode)
{
	if (mode != DEBUG_MODE && mode != UPDATE_MODE && mode != REQUEST_MODE) {
		debug_msg("unknown mode in send_package");
		return;
	}
	if (strlen(command) > 255) {
		debug_msg("command to long in send_package");
	}
	char length = strlen(command);
	data_out(&length, 1);
	data_out(&mode, 1);
	data_out(command, length);
}

void debug_msg(char *msg)
{
	send_package("DEBUG: ", DEBUG_MODE);
	send_package(msg, DEBUG_MODE);
	send_package("\n", DEBUG_MODE);
}

#else

int add_cmd(char *cmd_name, void (*func) (char *args));
int parse_cmd(char *cmd);
void cmd_help(char *args);
void cmd_status(char *args);
void cmd_servo_cal(char *args);
void cmd_servo(char *args);

struct cmd {
	void (*func) (char *args);
	char *name;
	void *next;
};

static int nr_of_cmds;
static struct cmd *first_cmd;

void debug_msg(char *msg)
{
	tprintf("DEBUG: ");
	tprintf(msg);
	tprintf("\n");
}

void serial_task(void *pvParameters)	//term-task
{
	char crrnt_cmd[TERM_CMD_LENGTH];
	int crrnt_cmd_i = 0;

	tprintf("FreeRTOS RC-CAR ---- chrisudeussen@gmail.com\n");

	add_cmd("help", cmd_help);
	add_cmd("status", cmd_status);
	add_cmd("servo_cal", cmd_servo_cal);
	add_cmd("servo", cmd_servo);

	char ch;

	tprintf("\n$");
	for (;;) {
		xQueueReceive(uart_receive_queue, &ch, portMAX_DELAY); //blocks
		switch (ch) {
		case '\b':
			if (crrnt_cmd_i > 0) {
				tprintf("%c %c", ch, ch);
				crrnt_cmd_i--;
			}
			break;
		case '\n':
		case '\r':
			tprintf("\n");
			if (crrnt_cmd_i > 0) {
				crrnt_cmd[crrnt_cmd_i] = 0x00;
				if (parse_cmd(crrnt_cmd)) {
					tprintf("unknown cmd\n");
				}
			}
			tprintf("$");
			crrnt_cmd_i = 0;
			break;
		default:
			tprintf("%c", ch);
			crrnt_cmd[crrnt_cmd_i] = ch;
			if (crrnt_cmd_i < TERM_CMD_LENGTH - 1) {
				crrnt_cmd_i++;
			}
		}
	}
}

void cmd_help(char *args)
{
	tprintf("available commands are:\n");
	tprintf("help - show this help\n");
	tprintf("status - show statistics, free Mem, and running tasks\n");
	tprintf("servo <servo_nr> <val> - set servo_nr to val\n");
	tprintf("servo_cal - calibrate servos to middle position\n");
	return;
}

void cmd_status(char *args)
{
	int free_heap = xPortGetFreeHeapSize();
	tprintf("Free Heap: %i of %i bytes used; %i%% full\n",
		free_heap, configTOTAL_HEAP_SIZE,
		((100 - free_heap) * 100) / configTOTAL_HEAP_SIZE);
	//peripherals
	tprintf("%i Servo(s) are connected, Middle Value is %i\n", NR_OF_SERVOS,
		SERVO_MIDDLE);

	for (int i = 0; i < NR_OF_SERVOS; i++) {
		tprintf("Servo %i has val: %i\n", i, servo_get(i));
	}
}

void cmd_servo_cal(char *args)
{
	servo_cal();		//calibrate servos
	tprintf("postion set as middle position\n");
}

//char *args is <cmd_name> <arg1> <arg2> ...
#define ARG_LENGTH 15 //length of arg

void cmd_servo(char *args)
{
	char arg1[ARG_LENGTH];
	char arg2[ARG_LENGTH];

	int length = get_word(arg1, args, ARG_LENGTH); //get the cmd_name... not needed
	length += get_word(arg1, (args+length), ARG_LENGTH);
	if (!*arg1) {
		tprintf("wrong args\n");
		return;
	}

	length = get_word(arg2, (args+length), ARG_LENGTH);
	if (!*arg2) {
		tprintf("wrong args\n");
		return;
	}

	int servo_nr = atoi(arg1);
	int servo_val = atoi(arg2);

	tprintf("set servo nr %i to %i \n", servo_nr, servo_val);

	switch (servo_nr) {
	case 0:
		servo_nr = SERVO_PIN_0;
		break;
	case 1:
		servo_nr = SERVO_PIN_1;
		break;
	default:
		servo_nr = 99;
	}
	servo_set(servo_val, servo_nr);
}

int parse_cmd(char *cmd)
{
	char cmd_name[ARG_LENGTH];

	// get command name
	int length = get_word(cmd_name, cmd, ARG_LENGTH);
	if (length == 0) {
		return -1;
	}
	//get command function
	struct cmd *crrnt_cmd = first_cmd;
	for (int i = 0; i < nr_of_cmds; i++) {
		if (strcmp(cmd_name, crrnt_cmd->name) == 0) {
			crrnt_cmd->func(cmd);
			return 0;
		}
		crrnt_cmd = crrnt_cmd->next;
	}
	return -1;
}

int add_cmd(char *cmd_name, void (*func) (char *args))
{
	struct cmd *crrnt_cmd;

	if (nr_of_cmds == 0) {
		first_cmd = (struct cmd *)pvPortMalloc(sizeof(struct cmd));
		if (first_cmd == NULL) {
			return -1;
		}
		crrnt_cmd = first_cmd;
	} else {
		crrnt_cmd = first_cmd;
		for (int i = nr_of_cmds - 1; i > 0; i--) {
			crrnt_cmd = crrnt_cmd->next;
		}
		crrnt_cmd->next =
		    (struct cmd *)pvPortMalloc(sizeof(struct cmd));
		if (crrnt_cmd->next == NULL) {
			return -1;
		}

		crrnt_cmd = crrnt_cmd->next;
	}

	crrnt_cmd->name = cmd_name;
	crrnt_cmd->func = func;
	crrnt_cmd->next = NULL;

	nr_of_cmds++;
	return 0;
}
#endif				// USE_TERMINAL

int get_word(char *buffer, char *source, const int length)
{
	int currnt_length = 0;
	while (*source == ' ')
		source += sizeof(char);
	while (*source != 0x00 && *source != ' ' && currnt_length < length) {
		*buffer = *source;
		buffer += sizeof(char);
		source += sizeof(char);
		currnt_length ++;
	}
	*buffer = 0x00;
	return  currnt_length;
}

