#include "car_status.h"

static struct car_val left;
static struct car_val right;
static struct car_val forward;
static struct car_val backward;
static struct car_val lights;
static struct car_val debug;
static struct car_val temp;
static struct car_val speed;
static struct car_val battery;

void status_init()
{
	car_vals = &left;

	left.name = "left";
	left.remote_controlled = 1;
	left.next = &right;

	right.name = "right";
	right.remote_controlled = 1;
	right.next = &forward;

	forward.name = "forward";
	forward.remote_controlled = 1;
	forward.next = &backward;

	backward.name = "backward";
	backward.remote_controlled = 1;
	backward.next = &lights;

	lights.name = "lights";
	lights.remote_controlled = 1;
	lights.next = &debug;

	debug.name = "debug";
	debug.remote_controlled = 1;
	debug.next = &temp;

	temp.name = "temp";
	temp.remote_controlled = 0;
	temp.next = &speed;

	speed.name = "speed";
	speed.remote_controlled = 0;
	speed.next = &battery;

	battery.name = "battery";
	battery.remote_controlled = 0;
	battery.next = 0x00000000;
}

void status_update_var(char *name, int val)
{
	struct car_val *current_val = car_vals;
	int string_i = 0;
	while (string_i < 250 && current_val != 0x00) {
		if (strcmp(current_val->name, name) == 0) {
			current_val->val = val;
			return;
		}
		current_val = current_val->next;
	}

}

//get var string of the remote_controlled vars
void status_get_var_str(char *buffer)
{
	struct car_val *current_val = car_vals;
	int string_i = 0;
	while (string_i < 250 && current_val != 0x00) {
		if (current_val->remote_controlled == 0) {
			strcpy((buffer + string_i), current_val->name);
			string_i += strlen(current_val->name);
			strcpy((buffer + string_i), " ");
			string_i++;
			itoa(current_val->val, (buffer + string_i), 10);
			string_i++;
			strcpy((buffer + string_i), " ");
			string_i++;
		}
		current_val = current_val->next;
	}
}
