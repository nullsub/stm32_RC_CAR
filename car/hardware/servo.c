#include <FreeRTOS.h>
#include "semphr.h"

#include "servo.h"
#include <string.h> // memcpy

#define TIME_100_US 	15	//0.1ms timer value
#define TIME_1_MS 	TIME_100_US*10	//1ms timer value

#define SERVO_UPDATE_TIME_MS 19	// the update period of the servos. this time is not critical

#define ALL_SERVO_PINS (SERVO_PIN_0 |  SERVO_PIN_1)

struct servo {
	int pin;
	unsigned int calibration;
	unsigned int time;
	volatile unsigned int int_time;
	/*when servo_update is called, this is copied 
	   into servo_state_INT, this done to ensure that interrupts are enabled as long as possible */
};

struct servo servos[NR_OF_SERVOS];

static inline void swap_servos(struct servo *x, struct servo *y)
{
	struct servo temp;
	memcpy(&temp, x, sizeof(struct servo));
	memcpy(x, y, sizeof(struct servo));
	memcpy(y, &temp, sizeof(struct servo));
}

void servo_init()
{
	servos[0].pin = SERVO_PIN_0;
	servos[1].pin = SERVO_PIN_1;

	for (int i = 0; i < NR_OF_SERVOS; i++) {
		servos[i].calibration = 0;
		servos[i].time = SERVO_MIDDLE * TIME_100_US;
	}
	servo_set(SERVO_MIDDLE, servos[0].pin);

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
	if (val < 10 || val > 20) {
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

	taskDISABLE_INTERRUPTS();

	servos[index].time = val * TIME_100_US + servos[index].calibration;	// 10 is left, 20 is right , 15 is middle!

	for (int i = 0; i < NR_OF_SERVOS; i++) {
		servos[i].int_time = servos[i].time;	//reset
	}

	if (NR_OF_SERVOS > 1) {
		for (int i = 0; i < (NR_OF_SERVOS - 1); i++) {
			for (int j = 0; j < (NR_OF_SERVOS - (i + 1)); j++) {
				if (servos[j].time > servos[j + 1].time) {
					swap_servos(&servos[j], &servos[j + 1]);
				}
			}
		}

		unsigned int length = 0;
		for (int i = 1; i < NR_OF_SERVOS; i++) {	//ordering now with offset
			length += servos[i - 1].int_time;
			servos[i].int_time -= length;
		}
	}

	taskENABLE_INTERRUPTS();
}

unsigned int servo_get(unsigned int index)
{
	if (index >= NR_OF_SERVOS) {
		return 9999;
	}

	return servos[index].time / TIME_100_US - servos[index].calibration;
}

#define START_PULSE 1
#define STOP_PULSE 0

void tim2_isr(void)		//__attribute__ ((interrupt)) this interrupt breaks if it is optimized!!!!!!!!!!!!
{
	static int state = START_PULSE;
	static unsigned int crrnt_servo;
	uint16_t next_time = SERVO_UPDATE_TIME_MS * TIME_1_MS;

	if (state == START_PULSE) {
		SERVO_PORT->BSRR = ALL_SERVO_PINS;	// can take full mask of all bits!
		next_time = servos[0].int_time;
		crrnt_servo = 0;
		state = STOP_PULSE;
	} else if (state == STOP_PULSE) {
		SERVO_PORT->BRR = servos[crrnt_servo].pin;

		crrnt_servo++;
		if (crrnt_servo < NR_OF_SERVOS) {	// "not the last"
			next_time = servos[crrnt_servo].int_time;
			while (next_time == 0 && crrnt_servo < NR_OF_SERVOS) {
				SERVO_PORT->BRR = servos[crrnt_servo].pin;
				crrnt_servo++;
				if (crrnt_servo < NR_OF_SERVOS)
					next_time =
					    servos[crrnt_servo].int_time;
			}
		}

		if (crrnt_servo >= NR_OF_SERVOS) {
			state = START_PULSE;
			next_time = SERVO_UPDATE_TIME_MS * TIME_1_MS;
		}
	}

	TIM2->ARR = next_time;

	TIM_ClearFlag(TIM2, TIM_FLAG_Update);	//Interrupt Flag von TIM2 Löschen
}

void servo_cal()
{
	for (int i = 0; i < NR_OF_SERVOS; i++) {
		servos[i].calibration =
		    (servos[i].time / TIME_100_US) - SERVO_MIDDLE;
	}
}
