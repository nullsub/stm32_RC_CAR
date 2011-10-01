#ifndef TEMPERATURE_C
#define TEMPERATURE_C

#include "temperature.h"
#include <stdlib.h>		// itoa
    
/* STM32 includes */ 
#include <stm32f10x.h>
#include <stm32f10x_conf.h>
void init_temperature()
{
	
/*	I2C_InitTypeDef i2c_init;

	I2C_StructInit(&i2c_init);
	
	
	i2c_init.I2C_ClockSpeed = 
	i2c_init.I2C_Mode = 
	i2c_init.I2C_DutyCycle = 
	i2c_init.I2C_OwnAddress1 = 
	i2c_init.I2C_Ack = 
	i2c_init.I2C_AcknowledgedAddress =

	I2C_Init(I2C1, &i2c_init);

	I2C_Cmd();
*/ 
} void get_temperature(unsigned int adress, char *s)
{				// adress of the i2c ds1621 sensor
/*	unsigned char   TempH, TempL;
	adress &= 0x0E;
	i2c_start(DS1621_Write|adress);
	i2c_write(0xEE);
        i2c_stop();      
        i2c_start(DS1621_Write|adress);
        i2c_write(0xAA);
        i2c_start(DS1621_Read|adress);
        TempH = i2c_readAck();
	TempL = i2c_readNak();
        i2c_stop();
	itoa(TempH,s,10); // convert to string
	while(*s){
		s++; // goto end of string
	}
	*s++ = '.';
	if(TempL == 0x80){ 
		*s++ = '5';
		
	}
	else{
		*s++ = '0';
	}
	*s = 0x00; //mark endo of string
*/ 
}    
#endif	/*  */
