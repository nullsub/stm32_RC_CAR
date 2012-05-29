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

//* STM32 includes */
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
#include "serial_io.h"

#define MS_PER_SEC		1000
#define DEBOUNCE_DELAY		40
#define UART_SEND_QUEUE_SIZE	16	
#define UART_RECEIVE_QUEUE_SIZE 24

/* Function Prototypes */
static void setup_rcc(void);
static void setup_gpio(void);
static void setup_exti(void);
static void setup_usart(void);
static void setup_nvic(void);
static void main_noreturn(void) NORETURN;

static void button_task(void *pvParameters) NORETURN;
static void startup_task(void *pvParameters) NORETURN;

static void setup(void);

static void blink_toggle_blue(void);
static void blink_toggle_green(void);

xQueueHandle uart_send_queue;
xQueueHandle uart_receive_queue;

static xSemaphoreHandle debounce_sem;

enum button_state {
	BUTTON_STATE_UP,
	BUTTON_STATE_DOWN
};

static enum button_state button_state;
static uint8_t led_blue = 1;
static uint8_t led_green = 1;

unsigned char outbyte(unsigned char ch)
{
	/* Enable USART TXE interrupt */
	xQueueSendToBack(uart_send_queue, &ch, portMAX_DELAY);	//blocks!
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

	xTaskCreate(startup_task, (signed portCHAR *)"startup",
			configMINIMAL_STACK_SIZE * 1, NULL, tskIDLE_PRIORITY + 1,
			&task);
	assert_param(task);

	/* Start the FreeRTOS scheduler */
	vTaskStartScheduler();
	assert_param(NULL);
	while (1) ;
}

void startup_task(void *pvParameters)
{
	uart_send_queue = xQueueCreate(UART_SEND_QUEUE_SIZE, sizeof(unsigned char));
	assert_param(uart_send_queue);

	uart_receive_queue =
		xQueueCreate(UART_RECEIVE_QUEUE_SIZE, sizeof(unsigned char));
	assert_param(uart_receive_queue);

	vSemaphoreCreateBinary(debounce_sem);
	assert_param(debounce_sem);

	setup();
	servo_init();
	serial_init();

	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);	//enable receiving.

	xTaskHandle task;

	xTaskCreate(serial_task, (signed portCHAR *)"serial",
			configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 2,
			&task);
	assert_param(task);

	xTaskCreate(button_task, (signed portCHAR *)"button",
			configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1,
			&task);
	assert_param(task);

	vTaskDelete(NULL);
	for (;;);
}

static inline void setup()
{
	setup_rcc();
	setup_gpio();
	setup_exti();
	setup_usart();
	setup_nvic();
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
			RCC_APB1Periph_BKP | RCC_APB1Periph_TIM2,
			ENABLE);

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

	/* Configure UART rx pin */
	gpio_init.GPIO_Pin = GPIO_Pin_10;
	gpio_init.GPIO_Mode = GPIO_Mode_IN_FLOATING;	//GPIO_Mode_AF_PP;
	gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &gpio_init);

	//config LED pins and servo
	gpio_init.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9 | SERVO_PIN_0 | SERVO_PIN_1;	// do this in servo.c servo_init()
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

//Configures USART controller
void setup_usart(void)
{
	USART_InitTypeDef usart_init;

	usart_init.USART_BaudRate = 9600;
	usart_init.USART_WordLength = USART_WordLength_8b;
	usart_init.USART_StopBits = 1;
	usart_init.USART_Parity = USART_Parity_No;
	usart_init.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	usart_init.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART1, &usart_init);

	/* Enable the USART */
	USART_Cmd(USART1, ENABLE);
}

//Configure the nested vectored interrupt controller
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
}

//This function handles USART interrupt request.
void usart1_isr(void)
{
	portBASE_TYPE task_woken;
	unsigned char ch;

	if (USART_GetITStatus(USART1, USART_IT_TXE) != RESET) {
		if (xQueueReceiveFromISR(uart_send_queue, &ch, &task_woken))
			USART_SendData(USART1, ch);
		else
			USART_ITConfig(USART1, USART_IT_TXE, DISABLE);
	}
	if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) {
		ch = USART_ReceiveData(USART1);
		xQueueSendFromISR(uart_receive_queue, &ch, &task_woken);
		//USART_ITConfig(USART1, USART_IT_RXNE, DISABLE);
	}
	portEND_SWITCHING_ISR(task_woken);
}

//This function handles External line 0 interrupt request.
void exti0_isr(void)
{
	static signed portBASE_TYPE task_woken;

	/* Clear pending bit */
	EXTI_ClearITPendingBit(EXTI_Line0);

	xSemaphoreGiveFromISR(debounce_sem, &task_woken);
	portEND_SWITCHING_ISR(task_woken);
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
		} else {
			volatile uint8_t button =
				GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0);

			if (button_state == BUTTON_STATE_UP && button) {
				button_state = BUTTON_STATE_DOWN;
				blink_toggle_blue();
				debug_msg("bprssd");
			} else if (button_state == BUTTON_STATE_DOWN && !button) {
				button_state = BUTTON_STATE_UP;
				debug_msg("brlsd");
			}

			debounce = 0;
			delay = portMAX_DELAY;
		}
	}
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

