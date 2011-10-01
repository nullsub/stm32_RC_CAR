#ifndef CAR_STATUS_H_
#define CAR_STATUS_H_

#define STEERING_INDEX 0 
#define ACCEL_INDEX	1
#define LIGHTS_INDEX	2	
#define DEBUG_INDEX	3
#define TEMP_INDEX	4
#define SPEED_INDEX	5
#define BATTERY_INDEX	6

struct car_val {
	char *name;
	int index;
	int val;
	int remote_controlled;	//whether this var is changed by the remote controller
	struct car_val *next;
};

struct car_val *car_vals;

void status_update_var(int index, int val);
void status_set_var(char *name, int val);
void status_init();
void status_get_var_str(char *buffer);

#endif				// CAR_STATUS_H
