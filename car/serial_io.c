#include "serial_io.h"
#include "car_status.h"
#include "tprintf.h"
#include <string.h>

#ifdef USE_TERMINAL
#include "servo.h"
#endif

#include <queue.h>

extern xQueueHandle tprintf_queue;
extern xQueueHandle uart_receive_queue;

void itoa(int z, char *Buffer, int base_NOT_USED_ALWAYS_TEN)
{
	int i = 0;
	int j;
	char tmp;
	unsigned u;		// In u bearbeiten wir den Absolutbetrag von z.

	// ist die Zahl negativ?
	// gleich mal ein - hinterlassen und die Zahl positiv machen
	if (z < 0) {
		Buffer[0] = '-';
		Buffer++;
		// -INT_MIN ist idR. größer als INT_MAX und nicht mehr 
		// als int darstellbar! Man muss daher bei der Bildung 
		// des Absolutbetrages aufpassen.
		u = ((unsigned)-(z + 1)) + 1;
	} else {
		u = (unsigned)z;
	}
	// die einzelnen Stellen der Zahl berechnen
	do {
		Buffer[i++] = '0' + u % 10;
		u /= 10;
	} while (u > 0);

	// den String in sich spiegeln
	for (j = 0; j < i / 2; ++j) {
		tmp = Buffer[j];
		Buffer[j] = Buffer[i - j - 1];
		Buffer[i - j - 1] = tmp;
	}
	Buffer[i] = '\0';
}

int atoi(char *c)
{
	int res = 0;
	while (*c >= '0' && *c <= '9')
		res = res * 10 + *c++ - '0';
	return res;
}

#ifndef USE_TERMINAL

static void handle_package(char *command, char mode);
static void send_all_vals();
static int get_word(char *buffer, char *source);
static void send_package(char *command, char mode);

void serial_task(void *pvParameters)	//remote_command_task
{
	char command[MAX_COMMAND_LENGTH];
	uint8_t length = 0;
	char mode = 0;
	char ch;

	status_init();

	for (;;) {
		xQueueReceive(uart_receive_queue, &length, portMAX_DELAY);	// get length
		xQueueReceive(uart_receive_queue, &mode, portMAX_DELAY);	// mode 
		for (int i = 0; i < length; i++) {
			xQueueReceive(uart_receive_queue, &ch, portMAX_DELAY);	// it blocks 
			command[i] = ch;
		}
		command[length] = 0x00;
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
			char *name;
			char *val;
			while (string_i < length) {
				string_i +=
				    get_word(name, (command + string_i));
				string_i += get_word(val, (command + string_i));
				status_update_var(name, atoi(val));
			}
		}
		break;
	}
	return;
}

int get_word(char *buffer, char *source)
{
	char *source_start = source;
	while (*source == ' ')
		source += sizeof(char);
	while (*source != 0x00 && *source != ' ') {
		*buffer = *source;
		buffer += sizeof(char);
		source += sizeof(char);
	}
	*buffer = 0x00;
	return (int)(source - source_start);
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
	uint8_t length = strlen(command);
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
		xQueueReceive(uart_receive_queue, &ch, portMAX_DELAY);	// it blocks 
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

struct cmd {
	void (*func) (char *args);
	char *name;
	void *next;
};
static int nr_of_cmds;
static struct cmd *first_cmd;

char *get_word(const char *string);
char *skip_word(char *string);

//char *args is <cmd_name> <arg1> <arg2> ...

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

void cmd_servo(char *args)
{
	char *arg1;
	char *arg2;

	args = skip_word(args);
	arg1 = get_word(args);
	if (arg1 == NULL) {
		tprintf("wrong args\n");
		return;
	}

	args = skip_word(args);
	arg2 = get_word(args);
	if (arg2 == NULL) {
		vPortFree(arg1);
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

	vPortFree(arg1);
	vPortFree(arg2);
}

char *skip_word(char *string)
{
	while (*string == ' ')
		string++;

	while (*string != ' ') {
		if (*string == 0x00)
			return NULL;
		string++;
	}
	return string;
}

char *get_word(const char *string)
{
	if (string == NULL)
		return NULL;

	//omit leading space
	char *start = (char *)string;
	while (*start == ' ')
		start++;
	if (strlen(start) < 1) {
		return NULL;
	}
	//find end
	int length = 0;
	while (*(start + sizeof(char) * length) != 0x00
	       && *(start + sizeof(char) * length) != ' ')
		length++;
	if (length < 1) {
		return NULL;
	}

	char *word = (char *)pvPortMalloc(length + 1);
	strncpy(word, start, length);
	word[length] = 0x00;
	return word;
}

int parse_cmd(char *cmd)
{
	char *cmd_name;

	// get command name
	cmd_name = get_word(cmd);
	if (cmd_name == NULL) {
		return -1;
	}
	//get command function
	struct cmd *crrnt_cmd = first_cmd;
	for (int i = 0; i < nr_of_cmds; i++) {
		if (strcmp(cmd_name, crrnt_cmd->name) == 0) {
			crrnt_cmd->func(cmd);
			vPortFree(cmd_name);
			return 0;
		}
		crrnt_cmd = crrnt_cmd->next;
	}

	vPortFree(cmd_name);
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
