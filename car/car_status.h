#ifndef CAR_STATUS_H_
#define CAR_STATUS_H_

struct car_val {
	char *name;
	int val;
	int remote_controlled;	//whether this var is changed by the remote controller
	void *next;
};

struct car_val *car_vals;

void status_update_var(char *name, int val);
void status_set_var(char *name, int val);
void status_init();
void status_get_var_str(char *buffer);

#endif				// CAR_STATUS_H
