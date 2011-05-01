
#include "terminal.h"
#include <string.h>

struct cmd{
	void (*func)(char *args);
	char *name;
	void * next;
};
static int nr_of_cmds;
static struct cmd *first_cmd;

void remove_leading_char(char * string, char character);




void cmd_help(char * args)
{
	return;
}

void cmd_status(char * args) 
{
	return;
}

void remove_leading_char(char * string, char character)
{
	int i;
	for(i = 0; i < strlen(string) && *(string+sizeof(char)*i) == character; i++);
	string = string+i*sizeof(char);
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
	if(strchr(cmd, ' ')) {
		int name_length = (strchr(cmd, ' ') - cmd);
		cmd_name = (char *) pvPortMalloc(name_length); 	
		strncpy(cmd_name, cmd, name_length);
		cmd_name[name_length+1] = 0x00;

	} else {
		cmd_name = cmd;
	}
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
		first_cmd->next = (struct cmd*) pvPortMalloc(sizeof(struct cmd));
		crrnt_cmd = first_cmd;
	}
	else{
		crrnt_cmd = first_cmd;
		for(int i = nr_of_cmds-1; i > 0; i--) {
			crrnt_cmd = crrnt_cmd->next;
		}
		crrnt_cmd->next = (struct cmd*) pvPortMalloc(sizeof(struct cmd));
		crrnt_cmd = crrnt_cmd->next;
	}

	crrnt_cmd->name = cmd_name;
	crrnt_cmd->func = func;
	crrnt_cmd->next = NULL;

	nr_of_cmds ++;
	return 0;
}



