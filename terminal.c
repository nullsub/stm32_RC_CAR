
#include "terminal.h"
#include <string.h>

struct cmd{
	void (*func)(char *args);
	char *name;
	void * next;
};
static int nr_of_cmds;
static struct cmd *first_cmd;

char * get_word(const char * string);
char * skip_word(char *string);

//char *args is <cmd_name> <arg1> <arg2> ...

int atoi(char *c) {
	int res = 0;
	while (*c >= '0' && *c <= '9')
		res = res * 10 + *c++ - '0';
	return res;
}

void cmd_help(char * args)
{
	tprintf("available commands are:\n");
	tprintf("help - show this help\n");
	tprintf("status - show statistics, free Mem, and running tasks\n");
	tprintf("servo <servo_nr> <val> - set servo_nr to val\n");
	tprintf("servo_cal - calibrate servos to middle position\n");
	return;
}

void cmd_status(char * args) 
{
	int free_heap = xPortGetFreeHeapSize(); 
	tprintf("Free Heap: %i of %i bytes used; %i%%full\n",
			free_heap, configTOTAL_HEAP_SIZE, 
			((100-free_heap)*100)/configTOTAL_HEAP_SIZE);
	//peripherals
	tprintf("%i Servo(s) are connected, Middle Value is %i\n",NR_OF_SERVOS, SERVO_MIDDLE);

	for(int i = 0; i < NR_OF_SERVOS; i++) { 
		tprintf("Servo %i has val: %i\n", i, servo_get(i));
	}
}

void cmd_servo_cal(char *args) 
{
	servo_cal(); //calibrate servos
	tprintf("postion set as middle position\n");
}

void cmd_servo(char *args)
{		
	char *arg1;
	char *arg2;

	args = skip_word(args);
	arg1 = get_word(args); 
	if(arg1 == NULL) {
		tprintf("wrong args\n");
		return;
	}

	args = skip_word(args);
	arg2 = get_word(args);
	if(arg2 == NULL) {
		vPortFree(arg1);
		tprintf("wrong args\n");
		return;
	}

	int servo_nr = atoi(arg1);
	int servo_val = atoi(arg2);

	tprintf("set servo nr %i to %i \n",servo_nr, servo_val);
	servo_set(servo_val, servo_nr);

	vPortFree(arg1);
	vPortFree(arg2);
}

char * skip_word(char *string){
	while(*string == ' ')
		string ++;

	while(*string != ' ') {
		if(*string == 0x00)
			return NULL;
		string++;
	} 
	return string;		
}

char * get_word(const char * string)
{
	if(string == NULL)
		return NULL;

	//omit leading space
	char * start = (char *)string;
	while(*start == ' ') 
		start ++;
	if(strlen(start) < 1) {
		return NULL;
	}

	//find end
	int length = 0;
	while(*(start+sizeof(char)*length) != 0x00 && *(start+sizeof(char)*length) != ' ') 
		length ++;
	if(length < 1) {
		return NULL;
	}

	char * word = (char *) pvPortMalloc(length+1); 	
	strncpy(word, start, length);
	word[length] = 0x00;
	return word;
}

int parse_cmd(char * cmd)
{
	char *cmd_name;

	// get command name
	cmd_name = get_word(cmd);
	if(cmd_name == NULL){
		return -1;
	}

	//get command function
	struct cmd *crrnt_cmd = first_cmd;
	for(int i = 0; i < nr_of_cmds; i++){
		if(strcmp(cmd_name, crrnt_cmd->name) == 0) {
			crrnt_cmd->func(cmd);
			vPortFree(cmd_name);
			return 0;
		}
		crrnt_cmd = crrnt_cmd->next;
	}	

	vPortFree(cmd_name);
	return -1;
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




