#ifndef TEMPERATURE_H
#define TEMPERATURE_H

void init_temperature();
void get_temperature(unsigned int adress, char *s);	// adress of the i2c ds1621 sensor

#endif	
