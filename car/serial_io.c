#include "FreeRTOS.h"
#include "semphr.h"
#include "queue.h"

#include <string.h>
#include "tprintf.h"
#include "common.h"
#include "serial_io.h"
#include "car_status.h"
#include "servo.h"

extern xQueueHandle uart_receive_queue;

static int get_word(char *buffer, char *source, const int length)
{
	int currnt_length = 0;
	int skipped_chars = 0;
	while (*source == ' ') {
		source += sizeof(char);
		skipped_chars ++;
	}
	while (*source != 0x00 && *source != ' ' && currnt_length < length) {
		*buffer = *source;
		buffer += sizeof(char);
		source += sizeof(char);
		currnt_length ++;
	}
	*buffer = 0x00;
	return  currnt_length + skipped_chars;
}

#ifndef USE_TERMINAL

#define ARG_LENGTH	7 

/*
 *	Remote Cotrolling protocol:
 *	A package consists of:
 *	[length] + [mode] + [plaintext ascii command/message]
 *	where length and mode are one byte. Length may be 0!
 *	for the standart UPDATE_MODE package, the message text consinst of:
 *	[index_for_var_a][a_space][value_for_var_a][a_space].....up to a length of 255bytes!
 */

static void handle_package(uint8_t mode, char * command, uint8_t length);
static void send_all_vals();
static void send_package(char *data, uint8_t mode);

static xSemaphoreHandle send_mutex;
static xSemaphoreHandle debug_msg_mutex;

void serial_init()
{
	send_mutex = xSemaphoreCreateMutex();
	debug_msg_mutex = xSemaphoreCreateMutex();
	status_init();
}

void serial_task(void *pvParameters)	//remote_command_task
{
	char command[MAX_COMMAND_LENGTH];
	char length_str[4];
	uint8_t length;
	uint8_t mode;
	unsigned char ch;

	for (;;) {
		/* get length and mode */
		xQueueReceive(uart_receive_queue, &length_str[0], portMAX_DELAY);
		xQueueReceive(uart_receive_queue, &length_str[1], portMAX_DELAY);
		xQueueReceive(uart_receive_queue, &length_str[2], portMAX_DELAY);
		xQueueReceive(uart_receive_queue, &mode, portMAX_DELAY);	
		length = atoi(length_str);
		if(length >= MAX_COMMAND_LENGTH) {
			length = MAX_COMMAND_LENGTH-1;
			debug_msg("length is too long");
		}
		for (uint8_t i = 0; i < length; i++) {
			xQueueReceive(uart_receive_queue, &ch, portMAX_DELAY);	
			command[i] = ch;
		}
		command[length] = 0x00;
		handle_package(mode, command, length);
	}
}

void handle_package(uint8_t mode, char * command, uint8_t length)
{
	char index[ARG_LENGTH];
	char val[ARG_LENGTH];
	switch (mode) {
		case UPDATE_MODE:	//Remote app sends updated vars
			for(unsigned int string_i = 0; string_i < length-2;) {
				string_i += get_word(index, (command+string_i), ARG_LENGTH);
				if(*index == 0x00) 
					return;
				string_i += get_word(val, (command + string_i), ARG_LENGTH);
				status_update_var(atoi(index), atoi(val));
			}
			break;
		case DEBUG_MODE:
			debug_msg("debug_mode in device");
			break;
		case REQUEST_MODE:	// remote app requests vars.
			send_all_vals();
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
	send_package(command, UPDATE_MODE);
}

void send_package(char *data, unsigned char mode)
{
	if (mode != DEBUG_MODE && mode != UPDATE_MODE && mode != REQUEST_MODE) {
		debug_msg("unknown mode in send_package");
		return;
	}

	unsigned int length = strlen(data);
	char length_str[4];

	/* need leading Zeros for the protocol*/
	if(length < 10) { 
		length_str[0] = '0';
		length_str[1] = '0';
		itoa(length, &length_str[2], 10);
	}
	else if(length < 100) {
		length_str[0] = '0';
		itoa(length, &length_str[1], 10);
	}
	else if(length < 1000) {
		itoa(length, &length_str[0], 10);
	}
	else {
		debug_msg("data to long in send_package");
		return;
	}

	xSemaphoreTake(send_mutex, portMAX_DELAY);
	data_out(length_str, 3);
	data_out((char *)&mode, 1);
	data_out(data, length);
	xSemaphoreGive(send_mutex);
}

void debug_msg(char *msg)
{
	xSemaphoreTake(debug_msg_mutex, portMAX_DELAY);	
	send_package(msg, DEBUG_MODE);
	xSemaphoreGive(debug_msg_mutex);	
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

void serial_init()
{

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
		xQueueReceive(uart_receive_queue, &ch, portMAX_DELAY); //blocking
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
					if (parse_cmd(crrnt_cmd))
						tprintf("unknown cmd\n");
				}
				tprintf("$");
				crrnt_cmd_i = 0;
				break;
			default:
				tprintf("%c", ch);
				crrnt_cmd[crrnt_cmd_i] = ch;
				if (crrnt_cmd_i < TERM_CMD_LENGTH - 1)
					crrnt_cmd_i++;
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
			((free_heap*100)/configTOTAL_HEAP_SIZE));
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
#define ARG_LENGTH	15 

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

