#include "car_status.h"
#include "servo.h"
#include "common.h"

static struct car_val steering;
static struct car_val accel;
static struct car_val lights;
static struct car_val debug;
static struct car_val temp;
static struct car_val speed;
static struct car_val battery;

void status_init()
{
	car_vals = &steering;

	steering.name = "steering";
	steering.index = STEERING_INDEX;
	steering.remote_controlled = 1;
	steering.next = &accel;

	accel.name = "accel";
	accel.index = ACCEL_INDEX;
	accel.remote_controlled = 1;
	accel.next = &lights;

	lights.name = "lights";
	lights.index = LIGHTS_INDEX;
	lights.remote_controlled = 1;
	lights.next = &debug;

	debug.name = "debug";
	debug.index = DEBUG_INDEX;
	debug.remote_controlled = 1;
	debug.next = &temp;

	temp.name = "temp";
	temp.index = TEMP_INDEX;
	temp.remote_controlled = 0;
	temp.next = &speed;

	speed.name = "speed";
	speed.index = SPEED_INDEX;
	speed.remote_controlled = 0;
	speed.next = &battery;

	battery.name = "battery";
	battery.index = BATTERY_INDEX;
	battery.remote_controlled = 0;
	battery.next = 0x00000000;
}

void status_update_var(int index, int val)
{
	struct car_val *current_val = car_vals;
	int string_i = 0;
	while (string_i < 250 && current_val != 0x00) {
		if (current_val->index == index) {
			current_val->val = val;
			if(current_val->remote_controlled == STEERING_INDEX) {
				servo_set(val, STEERING_SERVO);
			}
			return;
		}
		current_val = current_val->next;
	}

}

//get var string of the remote_controlled vars
void status_get_var_str(char *buffer)
{
	struct car_val *current_val = car_vals;
	while (current_val != 0x00) {
		if (current_val->remote_controlled == 0) {
			itoa(current_val->index, (buffer),10);
			while(*buffer)
				buffer ++;
			*buffer =  ' ';
			itoa(current_val->val, (buffer + 1), 10);
			while(*buffer)
				buffer ++;
			*buffer =  ' ';
			buffer ++;
		}
		current_val = current_val->next;
	}
}
