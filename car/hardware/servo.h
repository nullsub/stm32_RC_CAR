#ifndef SERVO_H_
#define SERVO_H_

/* FreeRTOS includes */
#include <FreeRTOS.h>
#include <task.h>

#include "common.h"

#define NR_OF_SERVOS	2
#define SERVO_PORT 	GPIOC
#define SERVO_PIN_0  	GPIO_Pin_0
#define SERVO_PIN_1  	GPIO_Pin_1
#define SERVO_PIN_2  	EOROROORORORORO

#define SERVO_MIDDLE 	15

void servo_init();
void servo_set(unsigned int val, int pin);
unsigned int servo_get(unsigned int index);
void servo_cal();

#endif
