
#include "terminal.h"
#include <string.h>
#include <stdlib.h>

struct cmd{
	void (*func)(char *args);
	char *name;
	void * next;
};
static int nr_of_cmds;
static struct cmd *first_cmd;

char * get_word(const char * string);
void remove_leading_char(char * string, char character);




void cmd_help(char * args)
{
	tprintf("available commands are:\r\n"
		"help - show this help\r\n"
		"status - show statistics, free Mem, and running tasks\r\n"
		"servo <servo_nr> <val> - set servo_nr to val\r\n"
		"servo_cal - calibrate servos to middle position\r\n"
		);
	return;
}

void cmd_status(char * args) 
{
	int free_heap = xPortGetFreeHeapSize(); 
	tprintf("Free Heap: %i of %i; %i%%full\r\n",
		 free_heap, configTOTAL_HEAP_SIZE, 
			((100-free_heap)*100)/configTOTAL_HEAP_SIZE);
	//peripherals
	tprintf("%i Servos are connected, Middle Value is %i\r\n",NR_OF_SERVOS, SERVO_MIDDLE);
	
	for(int i = 0; i < NR_OF_SERVOS; i++) { 
		tprintf("Servo %i has val: %i\r\n", i, servo_get(i));
	}

	return;
}

void cmd_servo_cal(char *args) 
{
	servo_cal(); //calibrate servos
}

void cmd_servo(char *args)
{		
	char *arg1;
	char *arg2;
	
	if(args == NULL) 
		return;

	arg1 = get_word(args);

	args = strchr(args, ' '); 
	if(args == NULL)
		return;
	remove_leading_char(args, ' ');

	arg2 = get_word(args);
	
	int servo_nr = atoi(arg1);
	int servo_val = atoi(arg2);

	tprintf("set servo nr %i to %i \r\n",servo_nr, servo_val);
	servo_set(servo_val, servo_nr);
	
	vPortFree(arg1);
	vPortFree(arg2);
}

void remove_leading_char(char * string, char character)
{
	int i;
	for(i = 0; i < strlen(string) && *(string+sizeof(char)*i) == character; i++);
	string = string+i*sizeof(char);
}

char * get_word(const char * string)
{
	if(strchr(string, ' ') == NULL)
		return (char *)string;
	int length = (strchr(string, ' ') - string);
	char *word = (char *) pvPortMalloc(length); 	
	strncpy(word, string, length);
	word[length+1] = 0x00;
	return word;
}

int parse_cmd(char * cmd)
{
	char *cmd_name;
	char *cmd_args;

	remove_leading_char(cmd, ' ');	

	if(strlen(cmd) < 1){
		return -1;
	}

	//get command args
	cmd_args = strchr(cmd, ' ');
	remove_leading_char(cmd_args, ' ');	

	// get command name
	cmd_name = get_word(cmd);

	//get command function
	struct cmd *crrnt_cmd = first_cmd;
	for(int i = 0; i < nr_of_cmds; i++){
		if(strcmp(cmd_name, crrnt_cmd->name) == 0) {
			crrnt_cmd->func(cmd_args);
			break;
		}
		crrnt_cmd = crrnt_cmd->next;
	}	


	vPortFree(cmd_name);
	vPortFree(cmd_args);

	return 0;
}

int add_cmd(char * cmd_name, void (*func)(char *args))
{
	struct cmd *crrnt_cmd;

	if(nr_of_cmds == 0){
		first_cmd = (struct cmd*) pvPortMalloc(sizeof(struct cmd));
		if(first_cmd == NULL ) {
			return -1;
		}
		crrnt_cmd = first_cmd;
	}
	else{
		crrnt_cmd = first_cmd;
		for(int i = nr_of_cmds-1; i > 0; i--) {
			crrnt_cmd = crrnt_cmd->next;
		}
		crrnt_cmd->next = (struct cmd*) pvPortMalloc(sizeof(struct cmd));
		if(crrnt_cmd->next == NULL ) {
			return -1;
		}

		crrnt_cmd = crrnt_cmd->next;
	}

	crrnt_cmd->name = cmd_name;
	crrnt_cmd->func = func;
	crrnt_cmd->next = NULL;

	nr_of_cmds ++;
	return 0;
}



