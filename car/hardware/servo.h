#ifndef SERVO_H_
#define SERVO_H_

/* STM32 includes */
#include <stm32f10x.h>
#include <stm32f10x_conf.h>

#include "common.h"

#define NR_OF_SERVOS	2
#define SERVO_PORT 	GPIOC
#define SERVO_PIN_0  	GPIO_Pin_0
#define SERVO_PIN_1  	GPIO_Pin_1
#define SERVO_PIN_2  	EOROROORORORORO

#define ALL_SERVO_PINS (SERVO_PIN_0 |  SERVO_PIN_1)

#define SERVO_MIDDLE 	225

#define STEERING_SERVO SERVO_PIN_1
#define ACCEL_SERVO SERVO_PIN_0

void servo_init();
void servo_set(unsigned int val, unsigned int index);
unsigned int servo_get(unsigned int index);
void servo_cal();

#endif
