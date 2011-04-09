#ifndef SERVO_H_
#define SERVO_H_

/* FreeRTOS includes */
#include <FreeRTOS.h>
#include <task.h>

#include "common.h"

#define NR_OF_SERVO 2 
#define SERVO_PORT GPIOC
#define SERVO_PIN_0  GPIO_Pin_0 
#define SERVO_PIN_1  GPIO_Pin_1
#define SERVO_PIN_2  EOROROORORORORO

#define SERVO_MIDDLE 10

enum servo_nr{
	STEERING_SERVO = 0,
	MOTOR_SERVO = 1
};

void servo_task(void *pvParameters) NORETURN;
void servo_init();
void servo_set(double val, enum servo_nr servo);

#endif

