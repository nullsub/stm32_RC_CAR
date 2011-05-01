
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

#define ALL_SERVO_PINS (SERVO_PIN_0 |  SERVO_PIN_1)

const int servo_pin[NR_OF_SERVOS] = {SERVO_PIN_0, SERVO_PIN_1};

static unsigned int servo_state[NR_OF_SERVOS]; 
/*when servo_update is called, this is copied 
  into servo_state_INT, this done to ensure that interrupts are enabled as long as possible*/

static volatile unsigned int servo_state_INT[NR_OF_SERVOS]; // used in the interrupt

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

	servo_state[servo_nr] = val*TIME_100_US;    		// 10 is left, 20 is right , 15 is middle!
}


unsigned int servo_get(unsigned int servo_nr)
{
	if(servo_nr >= NR_OF_SERVOS) {
		return 9999;
	}

	return servo_state[servo_nr];
}

static inline void swap(unsigned int *x, unsigned int *y)
{
	unsigned int temp;
	temp = *x;
	*x = *y;
	*y = temp;
}


void servo_update()
{
	// use bubble sort to update the order
	if(NR_OF_SERVOS > 1) {	
		for(int i = 0; i<(NR_OF_SERVOS-1);i++) {
			for(int j = 0; j<(NR_OF_SERVOS-(i+1));j++) {
				if(servo_state[j] > servo_state[j+1]) {
					swap(&servo_state[j],&servo_state[j+1]);
				}
			}
		}
	}
	
	int length = 0;
	for(int i = 1; i < NR_OF_SERVOS; i++) {  //ordering now with offset
		length += servo_state[i-1];
		servo_state[i] -= length; 	
	}	

	// I really dont know how to do this better!?	
	taskDISABLE_INTERRUPTS();
	for(int i = 0; i < NR_OF_SERVOS; i++) {
		servo_state_INT[i] = servo_state[i];
	}
	taskENABLE_INTERRUPTS();	
}

#define START_PULSE 1
#define STOP_PULSE 0

void tim2_isr(void) 			//__attribute__ ((interrupt)) this interrupt breaks if it is optimized!!!!!!!!!!!!
{
	static int state = START_PULSE;
	static unsigned int crrnt_servo;
	uint16_t next_time = SERVO_UPDATE_TIME_MS * TIME_1_MS;

	if(state == START_PULSE) {		
   		SERVO_PORT->BSRR = ALL_SERVO_PINS; // can take full mask of all bits!
		next_time = servo_state_INT[0];
		crrnt_servo = 0;
		state = STOP_PULSE;
	}
	else if( state ==  STOP_PULSE){
    		SERVO_PORT->BRR = servo_pin[crrnt_servo];
	
		crrnt_servo ++;
		if(crrnt_servo < NR_OF_SERVOS) { // "not the last"
			next_time = servo_state_INT[crrnt_servo];
			while(next_time == 0 && crrnt_servo < NR_OF_SERVOS) {
    				SERVO_PORT->BRR = servo_pin[crrnt_servo];
				crrnt_servo ++;
				if(crrnt_servo < NR_OF_SERVOS) 
					next_time = servo_state_INT[crrnt_servo];
			}	
		}

		if(crrnt_servo >= NR_OF_SERVOS){
			state = START_PULSE;
			next_time = SERVO_UPDATE_TIME_MS*TIME_1_MS;
		}
	}

	TIM2->ARR = next_time;

	TIM_ClearFlag(TIM2, TIM_FLAG_Update);         //Interrupt Flag von TIM2 Löschen
}

