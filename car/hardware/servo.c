#include "servo.h"

/* STM32 includes */
#include <FreeRTOS.h>
#include "semphr.h"
#include "task.h"

#define TIME_100_US 	15	//0.1ms timer value
#define TIME_1_MS 	TIME_100_US*10	//1ms timer value

#define SERVO_MAX	TIME_1_MS*2 	// right
#define SERVO_MIN	TIME_1_MS 	// left

#define SERVO_UPDATE_TIME_MS 19	// the update period of the servos. this time is not critical

#define ALL_SERVO_PINS (SERVO_PIN_0 |  SERVO_PIN_1)

struct servo {
	int pin;
	signed int calibration;
	unsigned int time;
};

struct servo servos[NR_OF_SERVOS];

void servo_init()
{
	servos[0].pin = SERVO_PIN_0;
	servos[1].pin = SERVO_PIN_1;

	for (int i = 0; i < NR_OF_SERVOS; i++) {
		servos[i].calibration = 0;
		servo_set(SERVO_MIDDLE, servos[i].pin);
	}

	TIM_TimeBaseInitTypeDef timer_settings;
	/* TIM2CLK = 24 MHz, Prescaler = 160, TIM2 counter clock = 150000 Hz 
	 * 24MHz / (160) = 150000Hz 
	 * 75 * (1/150000Hz) = 0.0005S overflow interrupt */

	/* Time base configuration */
	timer_settings.TIM_Period = 650;
	timer_settings.TIM_Prescaler = 160 - 1;
	timer_settings.TIM_ClockDivision = TIM_CKD_DIV1;
	timer_settings.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM2, &timer_settings);

	/* Clear TIM2 update pending flag */
	TIM_ClearFlag(TIM2, TIM_FLAG_Update);

	/* TIM IT enable */
	TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);

	TIM_ClearFlag(TIM2, TIM_FLAG_Update);

	TIM_Cmd(TIM2, ENABLE);
}

void servo_set(unsigned int val, int pin)
{
	if (val < SERVO_MIN || val > SERVO_MAX) {
		return;
	}
	if (pin == 99) {
		return;
	}

	int index = 0;

	for (int i = 0; i < NR_OF_SERVOS; i++) {
		if (servos[i].pin == pin) {
			index = i;
			break;
		}
	}
	servos[index].time = val + servos[index].calibration;	// 150 is left, 300 is right , 225 is middle!
}

unsigned int servo_get(unsigned int index)
{
	if (index >= NR_OF_SERVOS) {
		return 9999;
	}

	return (servos[index].time - servos[index].calibration);
}
void tim2_isr(void) __attribute__ ((optimize(0)));// this interrupt breaks if it is optimized!!!!!!!!!!!!
void tim2_isr(void)
{
	static unsigned int crrnt_servo = 0;
	static uint16_t crrnt_period = 0;
	static int start_flag = 1;
	const uint16_t update_period = SERVO_UPDATE_TIME_MS * TIME_1_MS;

	SERVO_PORT->BRR = servos[crrnt_servo].pin; //clear
		
	if(start_flag) {
		start_flag = 0;	
	}else {
		crrnt_servo ++;
	}
	if(crrnt_servo >= NR_OF_SERVOS){
		crrnt_servo = 0;
		if(crrnt_period < update_period-(TIME_1_MS*3)) { //too early to start the next refresh...
			start_flag = 1; 
			TIM2->ARR = update_period-crrnt_period; 
			crrnt_period = 0;
			TIM_ClearFlag(TIM2, TIM_FLAG_Update);	//Interrupt Flag von TIM2 Löschen
			return;
		}
		crrnt_period = 0;
	}

	SERVO_PORT->BSRR = servos[crrnt_servo].pin; //set

	crrnt_period += servos[crrnt_servo].time;
	TIM2->ARR = servos[crrnt_servo].time;

	TIM_ClearFlag(TIM2, TIM_FLAG_Update);	//Interrupt Flag von TIM2 Löschen
}

void servo_cal()
{
	for (int i = 0; i < NR_OF_SERVOS; i++) {
		servos[i].calibration =
		    (servos[i].time) - SERVO_MIDDLE;
	}
}
