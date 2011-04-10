
#include "servo.h"
#include <string.h>

/* STM32 includes */
#include <stm32f10x.h>
#include <stm32f10x_conf.h>

#include <FreeRTOS.h>
#include "semphr.h"

#define TIME_100_US 	15 		//0.1ms timer value
#define TIME_1_MS 	TIME_100_US*10 	//1ms timer value

#define SERVO_UPDATE_TIME_MS 19 	// the update period of the servos. this time is not critical 



const int servo_pin[NR_OF_SERVOS] = {SERVO_PIN_0};

static unsigned int servo_state[NR_OF_SERVOS]; 
/*when servo_update is called, this is copied 
into servo_state_INT, this done to ensure that interrupts are enabled as long as possible*/

static unsigned int servo_state_INT[NR_OF_SERVOS]; // used in the interrupt

void servo_init()
{
	for(int i = 0; i < NR_OF_SERVOS; i++)
	{
		servo_set(SERVO_MIDDLE, i);
	}

	servo_update();
	
}

void servo_set(unsigned int val, unsigned int servo_nr)
{
	if(val < 10 || val > 20 ){
		return;
	}
	if(servo_nr >= NR_OF_SERVOS) {
		return;
	}

	val = 250;	

	servo_state[servo_nr] = val*TIME_100_US;    		// 10 is left, 20 is right , 15 is middle!
}



static inline void swap(int *x,int *y)
{
   int temp;
   temp = *x;
   *x = *y;
   *y = temp;
}


void servo_update()
{
// use bubble sort to update the order
#if NR_OF_SERVOS > 1
/*	for(int i = 0; i<(NR_OF_SERVOS-1);i++) {
		for(int j = 0; j<(NR_OF_SERVOS-(i+1));j++) {
			if(servo_state[j] > servo_state[j+1]) {
				swap(&servo_state[j],&servo_state[j+1]);
			}
		}
	}
*/
#endif

	// I really dont know how to do this better!?	
//	taskDISABLE_INTERRUPTS();
	for(int i = 0; i < NR_OF_SERVOS; i++) {
		servo_state_INT[i] = servo_state[i];
	}
//	taskENABLE_INTERRUPTS();	
}

enum {
	START_PULSE,
	STOP_PULSE
};

void tim2_isr(void)
{
	static int state = START_PULSE;
	static unsigned int crrnt_servo;
	unsigned int next_time;

	switch (state) {
		case START_PULSE:		
			for(int i = 0; i < NR_OF_SERVOS; i++){
				GPIO_WriteBit(SERVO_PORT, servo_pin[crrnt_servo], 1); // start Pulse at the same time for all
			}
			next_time = servo_state_INT[0];
			crrnt_servo = 0;
			state = STOP_PULSE;
			break;
		case STOP_PULSE:
			do{
				next_time = servo_state_INT[crrnt_servo];
				GPIO_WriteBit(SERVO_PORT, servo_pin[crrnt_servo], 0); 
				crrnt_servo ++; // broken!
			} while(next_time == 0 && crrnt_servo < NR_OF_SERVOS);
			if(crrnt_servo >= NR_OF_SERVOS){
				state = START_PULSE;
				next_time = SERVO_UPDATE_TIME_MS*TIME_1_MS;
				break;
			}
			break;
	}

	TIM2->ARR = next_time;

	TIM_ClearFlag(TIM2, TIM_FLAG_Update);         //Interrupt Flag von TIM2 Löschen
}

