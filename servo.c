
#include "servo.h"
/* STM32 includes */
#include <stm32f10x.h>
#include <stm32f10x_conf.h>

#include <FreeRTOS.h>
#include "semphr.h"

#define TIME_100_US 15 //0.1ms timer value
#define TIME_100_MS TIME_100_US*10 //1ms timer value

static double servo_state[NR_OF_SERVO];

struct servo{
	int pin;
	double state;
};

static xSemaphoreHandle servo_cpy_mutex;




void servo_task(void *pvParameters)
{
	struct servo servos[NR_OF_SERVO];

	servo_cpy_mutex = xSemaphoreCreateMutex();

	servo_init();

	while(1){
		taskYIELD();	
	}
}



void servo_init()
{
	for(int i = 0; i < NR_OF_SERVO; i++)
	{
		servo_state[i] = SERVO_MIDDLE;
	}

}

void servo_set(double val, enum servo_nr servo)
{
	xSemaphoreTake(servo_cpy_mutex,  portMAX_DELAY);
	servo_state[servo] = val;
	xSemaphoreGive(servo_cpy_mutex);	
}

void tim2_isr(void)
{
	if(GPIO_ReadOutputDataBit(SERVO_PORT,SERVO_PIN_0) == 1) { 
		GPIO_WriteBit(SERVO_PORT,SERVO_PIN_0, 0); // start Pulse at the same time for all

		/* Set the Autoreload value */
		TIM2->ARR = TIME_100_US; // shortest time needed...  all testing
	}
	else {
		GPIO_WriteBit(SERVO_PORT,SERVO_PIN_0, 1); // start Pulse at the same time for all

		TIM2->ARR = TIME_1_MS * 20 ; // 20ms == longest time needed
	}

	TIM_ClearFlag(TIM2, TIM_FLAG_Update);         //Interrupt Flag von TIM2 Löschen
}

