/**
 ******************************************************************************
 *
 * @file       main.c
 * @author     Stephen Caudle Copyright (C) 2010
 * @brief      Main implementation
 * @see        The GNU Public License (GPL) Version 3
 *
 *****************************************************************************/
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

/* STM32 includes */
#include <stm32f10x.h>
#include <stm32f10x_conf.h>

/* FreeRTOS includes */
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <semphr.h>

/* Includes */
#include "common.h"
#include "tprintf.h"
#include "servo.h"
#include "terminal.h"

#define MS_PER_SEC		1000
#define DEBOUNCE_DELAY		40
#define TPRINTF_QUEUE_SIZE	16
#define RECEIVE_QUEUE_SIZE 	16

/* Function Prototypes */
static void setup_rcc(void);
static void setup_gpio(void);
static void setup_exti(void);
static void setup_usart(void);
static void setup_nvic(void);
static void main_noreturn(void) NORETURN;


static void button_task(void *pvParameters) NORETURN;
static void term_task(void *pvParameters) NORETURN;

static void setup(void);

static void blink_toggle_blue(void);
static void blink_toggle_green(void);


enum button_state
{
	BUTTON_STATE_UP,
	BUTTON_STATE_DOWN
};


static xQueueHandle tprintf_queue;
xQueueHandle uart_receive_queue;

static xSemaphoreHandle debounce_sem;

static enum button_state button_state;
static uint8_t led_blue = 1;
static uint8_t led_green = 1;

/**
 * @brief  Retargets the C library printf function to the USART.
 * @param  ch The character to print
 * @retval The character printed
 */
int outbyte(int ch)
{
	/* Enable USART TXE interrupt */
	xQueueSendToBack(tprintf_queue, &ch, 0);
	USART_ITConfig(USART1, USART_IT_TXE, ENABLE);
	return ch;
}

/**
 * Main function
 */
int main(void)
{
	main_noreturn();
}

inline void main_noreturn(void)
{
	xTaskHandle task;

	add_cmd("help", cmd_help);
	add_cmd("status", cmd_status);
	add_cmd("servo_cal", cmd_servo_cal);
	add_cmd("servo", cmd_servo);


	xTaskCreate(term_task, (signed portCHAR *)"terminal", configMINIMAL_STACK_SIZE * 2, NULL, tskIDLE_PRIORITY + 1, &task);
	assert_param(task);

	xTaskCreate(button_task, (signed portCHAR *)"button", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 2, &task);
	assert_param(task);


	/* Start the FreeRTOS scheduler */
	vTaskStartScheduler();
	assert_param(NULL);

	while (1);
}

static inline void setup()
{
	setup_rcc();
	setup_gpio();
	setup_exti();
	setup_usart();
	setup_nvic();

	tprintf("FreeRTOS RC-CAR ---- chrisudeussen@gmail.com\r\n");
}

/**
 * @brief  Configures the peripheral clocks
 * @param  None
 * @retval None
 */
void setup_rcc(void)
{
	/* Enable PWR clock */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR |
			RCC_APB1Periph_BKP | RCC_APB1Periph_TIM2
			, ENABLE);

	/* Enable GPIOA and GPIOC clock */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA |
			RCC_APB2Periph_GPIOC |
			RCC_APB2Periph_USART1 |
			RCC_APB2Periph_AFIO, ENABLE);

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE); 
}

/**
 * @brief  Configures the different GPIO ports
 * @param  None
 * @retval None
 */
void setup_gpio(void)
{
	GPIO_InitTypeDef gpio_init;

	/* Configure UART tx pin */
	gpio_init.GPIO_Pin = GPIO_Pin_9;
	gpio_init.GPIO_Mode = GPIO_Mode_AF_PP;
	gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &gpio_init);

	//config LED pins and servo
	gpio_init.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9 | SERVO_PIN_0 | SERVO_PIN_1;  // do this in servo.c servo_init()
	gpio_init.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOC, &gpio_init);

	/* Configure button input floating */
	gpio_init.GPIO_Pin = GPIO_Pin_0;
	gpio_init.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &gpio_init);

	/* Connect Button EXTI Line to Button GPIO Pin */
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource0);
}

/**
 * @brief  Configures EXTI Lines
 * @param  None
 * @retval None
 */
void setup_exti(void)
{
	EXTI_InitTypeDef EXTI_InitStructure;

	EXTI_ClearITPendingBit(EXTI_Line0);
	EXTI_InitStructure.EXTI_Line = EXTI_Line0;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);
}

/**
 * @brief  Configures USART controller
 * @param  None
 * @retval None
 */
void setup_usart(void)
{
	USART_InitTypeDef usart_init;

	usart_init.USART_BaudRate = 115200;
	usart_init.USART_WordLength = USART_WordLength_8b;
	usart_init.USART_StopBits = 1;
	usart_init.USART_Parity = USART_Parity_No;
	usart_init.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	usart_init.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART1, &usart_init);

	/* Enable the USART */
	USART_Cmd(USART1, ENABLE);
}

/**
 * @brief  Configure the nested vectored interrupt controller
 * @param  None
 * @retval None
 */
void setup_nvic(void)
{
	NVIC_InitTypeDef nvic_init;

	/* Set the Vector Table base address as specified in .ld file */
	NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0x0);

	/* 4 bits for Interupt priorities so no sub priorities */
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);

	/* Configure HCLK clock as SysTick clock source. */
	SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK);

	/* Configure USART interrupt */
	nvic_init.NVIC_IRQChannel = USART1_IRQn;
	nvic_init.NVIC_IRQChannelPreemptionPriority = 0xf;
	nvic_init.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&nvic_init);


	/* Configure EXTI interrupt */
	nvic_init.NVIC_IRQChannel = EXTI0_IRQn;
	nvic_init.NVIC_IRQChannelPreemptionPriority = 0xc;
	nvic_init.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&nvic_init);

	/* Configure Servo interrupt */
	nvic_init.NVIC_IRQChannel = TIM2_IRQn;
	nvic_init.NVIC_IRQChannelPreemptionPriority = 0x2;
	nvic_init.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&nvic_init);



	servo_init();


	//--------------- do this in sero_init....-------------------------------//
	TIM_TimeBaseInitTypeDef timer_settings; 
	/* TIM2CLK = 24 MHz, Prescaler = 160, TIM2 counter clock = 150000 Hz 
	 * 24MHz / (160) = 150000Hz 
	 * 75 * (1/150000Hz) = 0.0005S overflow interrupt */ 

	/* Time base configuration */ 
	timer_settings.TIM_Period = 650;  	
	timer_settings.TIM_Prescaler = 160-1; 
	timer_settings.TIM_ClockDivision = TIM_CKD_DIV1; 
	timer_settings.TIM_CounterMode = TIM_CounterMode_Up; 
	TIM_TimeBaseInit(TIM2, &timer_settings); 

	/* Clear TIM2 update pending flag */ 
	TIM_ClearFlag(TIM2, TIM_FLAG_Update); 

	/* TIM IT enable */ 
	TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE); 

	TIM_ClearFlag(TIM2, TIM_FLAG_Update); 

	TIM_Cmd(TIM2, ENABLE); 
	//-----------------------------------------------------------//

}

/**
 * @brief  This function handles USART interrupt request.
 * @param  None
 * @retval None
 */
void usart1_isr(void)
{
	portBASE_TYPE task_woken;
	unsigned char ch;

	if (USART_GetITStatus(USART1, USART_IT_TXE) != RESET) {
		unsigned char ch;

		if (xQueueReceiveFromISR(tprintf_queue, &ch, &task_woken))
			USART_SendData(USART1, ch);
		else
			USART_ITConfig(USART1, USART_IT_TXE, DISABLE);
	}
	if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) {	
		ch = USART_ReceiveData(USART1);

		xQueueSendFromISR(uart_receive_queue, &ch, &task_woken);

	}
	portEND_SWITCHING_ISR(task_woken);
}

/**
 * @brief  This function handles External line 0 interrupt request.
 * @param  None
 * @retval None
 */
void exti0_isr(void)
{
	static signed portBASE_TYPE task_woken;

	/* Clear pending bit */
	EXTI_ClearITPendingBit(EXTI_Line0);

	xSemaphoreGiveFromISR(debounce_sem, &task_woken);
	portEND_SWITCHING_ISR(task_woken);
}

void blink_toggle_blue()
{
	GPIO_WriteBit(GPIOC, GPIO_Pin_8, led_blue);
	led_blue ^= 1;
}

void blink_toggle_green()
{
	GPIO_WriteBit(GPIOC, GPIO_Pin_9, led_green);
	led_green ^= 1;
}

void term_task(void *pvParameters) 	// Terminal Task
{
	char crrnt_cmd[TERM_CMD_LENGTH];
	int crrnt_cmd_i = 0;	

	tprintf_queue = xQueueCreate(TPRINTF_QUEUE_SIZE, sizeof(unsigned char));
	assert_param(tprintf_queue);

	uart_receive_queue = xQueueCreate(RECEIVE_QUEUE_SIZE, sizeof(unsigned char));
	assert_param(uart_receive_queue);

	vSemaphoreCreateBinary(debounce_sem);
	assert_param(debounce_sem);

	setup();

	char ch;

	for (;;) {
		xQueueReceive(uart_receive_queue, &ch, portMAX_DELAY); // does it block?
		if(ch == '\n' || ch == '\r'){
			crrnt_cmd[crrnt_cmd_i] = 0x00;
			parse_cmd(crrnt_cmd);
			crrnt_cmd_i = 0;
		} else {
			crrnt_cmd[crrnt_cmd_i] = ch;
			if(crrnt_cmd_i < TERM_CMD_LENGTH-1) {
				crrnt_cmd_i ++;
			}
		}
	}
}

void button_task(void *pvParameters)
{
	portTickType delay = portMAX_DELAY;
	uint8_t debounce = 0;

	blink_toggle_blue();

	for (;;) {
		if (xSemaphoreTake(debounce_sem, delay) == pdTRUE) {
			if (!debounce) {
				debounce = 1;
				delay = DEBOUNCE_DELAY;
			}
		}
		else {
			volatile uint8_t button = GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0);

			if (button_state == BUTTON_STATE_UP && button) {
				button_state = BUTTON_STATE_DOWN;
				blink_toggle_blue();
				tprintf("button press\r\n");
			}
			else if (button_state == BUTTON_STATE_DOWN && !button) {
				button_state = BUTTON_STATE_UP;
				tprintf("button release\r\n");
			}

			debounce = 0;
			delay = portMAX_DELAY;
		}
	}
}

