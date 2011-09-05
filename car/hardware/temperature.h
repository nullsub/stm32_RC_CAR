#ifndef TEMPERATURE_H
#define TEMPERATURE_H

//------defines---------
#define DS1621_Write  0x90 
#define DS1621_Read   0x91


void init_temperature();
void get_temperature(unsigned int adress, char * s); // adress of the i2c ds1621 sensor 


#endif
