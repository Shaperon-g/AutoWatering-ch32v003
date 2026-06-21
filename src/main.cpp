/* configurations */
#define AWTR_UPDATE_TIME 12   // [1 - 40 hours]
#define AWTR_WATERING_TIME 2000   // [ms]

/* clock preferences. Don't change thoughtlessly! */
#define F_HBCLK 187500
#define UART_BAUDRATE 4800

#define MS_TO_TICKS(ms) ((uint32_t)(ms) * (F_HBCLK / 8000))
#define TIMER_UPDATE MS_TO_TICKS(AWTR_UPDATE_TIME * 3600 * 1000)

/* Includes */
#include <ch32v00x.h>
#include <uart.h>
#include "GPIO.h"
#include "ASCII_fun.h"

/* Global Variable */
enum awtr_state_t {
	blink,
	waiting,
	click,
	hold,
	long_hold,
	toSleep,
};
volatile awtr_state_t awtr_state = click;
uint8_t sens_val = 0;
uint8_t sens_val_border = 50;
#define BORDER ((uint32_t*)0x08003FC0)

uart uart1(USART1, UART_BAUDRATE, F_HBCLK);
uint8_t msg_click_dry[] = "Ground is dry: XXX\n";
uint8_t msg_click_wet[] = "Ground is wet: XXX\n";
uint8_t msg_hold[] = "(X) sens border: XXX\n";
uint8_t msg_lhold[] = "Sensor value: XXX\n";

/* Program */
void delayT(uint32_t val) {       // [val] ~= 0.042 sec
	uint32_t cnt_prev, cnt_cur;
	cnt_prev = SysTick->CNT;
	cnt_cur = SysTick->CNT;
	while(cnt_cur < cnt_prev + val) {
		cnt_cur = SysTick->CNT;
	}
	return;
}

uint8_t flash_get() {
	return *BORDER;
}
uint8_t flash_save(uint8_t data) {
	__disable_irq();
	FLASH->KEYR = 0x45670123;
	FLASH->KEYR = 0xCDEF89AB;
	if(FLASH->CTLR & FLASH_CTLR_LOCK) {
		return 1;
	}
	FLASH->MODEKEYR = 0x45670123;
	FLASH->MODEKEYR = 0xCDEF89AB;
	if(FLASH->CTLR & FLASH_CTLR_FLOCK) {
		return 1;
	}

	FLASH->CTLR |= FLASH_CTLR_PAGE_ER;
	FLASH->ADDR = 0x08003FC0;
	FLASH->CTLR |= FLASH_CTLR_STRT;
	while(FLASH->STATR & FLASH_STATR_BSY) {;}
	FLASH->CTLR &= ~(FLASH_CTLR_PAGE_ER);

	FLASH->CTLR |= FLASH_CTLR_PAGE_PG;
	FLASH->CTLR |= FLASH_CTLR_BUF_RST;
	while(FLASH->STATR & FLASH_STATR_BSY) {;}
	*BORDER = (uint32_t)data;
	FLASH->CTLR |= FLASH_CTLR_BUF_LOAD;
	while(FLASH->STATR & FLASH_STATR_BSY) {;}
	FLASH->ADDR = 0x08003FC0;
	FLASH->CTLR |= FLASH_CTLR_STRT;
	while(FLASH->STATR & FLASH_STATR_BSY) {;}
	FLASH->CTLR &= ~(FLASH_CTLR_PAGE_PG);
	FLASH->CTLR |= FLASH_CTLR_LOCK;
	if(FLASH->STATR & FLASH_STATR_WRPRTERR) {
		return 1;
	}
	__enable_irq();
	return 0;
}

extern "C"{
void SystemInit() {
	/*RCC*/
	RCC->CFGR0 &= ~(RCC_HPRE | RCC_ADCPRE);
	RCC->CFGR0 |= RCC_HPRE_DIV16 | RCC_ADCPRE_DIV2; //HBCLK = 1.5 MHz
	RCC->APB2PCENR |= RCC_IOPAEN | RCC_IOPCEN | RCC_IOPDEN | 
	                  RCC_ADC1EN | RCC_USART1EN | RCC_AFIOEN;
	
	/*Timer*/
	SysTick->CNT = 0;
	SysTick->CMP = 0xFFFFFFFF;
	SysTick->CTLR = STK_STIE | STK_STRE | STK_STE;
	
	/*GPIO*/
	LED_PORT->CFGLR &= ~(IO_MASK << LED_GPIN);
	LED_PORT->CFGLR |= (IO_OUT_OD | IO_OUT_2M) << LED_GPIN;
	led_off();

	PUMP_PORT->CFGLR &= ~(IO_MASK << PUMP_GPIN);
	PUMP_PORT->CFGLR |= (IO_OUT_PP | IO_OUT_2M) << PUMP_GPIN;

	BTN_PORT->CFGLR &= ~(IO_MASK << BTN_GPIN);
	BTN_PORT->CFGLR |= (IO_IN | IO_IN_PUD) << BTN_GPIN;
	BTN_PORT->BSHR = 1 << btn_pin;  //pull-up
	AFIO->EXTICR |= BTN_EXTI;
	EXTI->INTENR |= 1 << btn_pin;
	EXTI->FTENR |= 1 << btn_pin;
	EXTI->RTENR |= 1 << btn_pin;
	EXTI->INTFR |= 1 << btn_pin;
	EXTI->EVENR |= 1 << btn_pin;

	SENS_PORT->CFGLR &= ~(IO_MASK << SENS_GPIN);
	SENS_PORT->CFGLR |= (IO_IN | IO_IN_AN) << SENS_GPIN;

	UTX_PORT->CFGLR &= ~(IO_MASK << UTX_GPIN);
	UTX_PORT->CFGLR |= (IO_OUT_APP | IO_OUT_10M) << UTX_GPIN;
	AFIO->PCFR1 |= UTX_REMAP;

	/*ADC*/
	ADC1->CTLR1 |= 0;  //Single single-channel mode
	ADC1->CTLR2 |= ADC_ALIGN | ADC_ADON;
	ADC1->SAMPTR2 = 0;
	ADC1->RSQR1 = 0;
	ADC1->RSQR2 = 0;
	ADC1->RSQR3 = SENS_ADC_CHANNEL << 0;

	/* RCC end*/
	delayT(MS_TO_TICKS(5000 * 8)); //for program chip (~ 5 sec)
	RCC->CFGR0 &= ~(RCC_HPRE | RCC_ADCPRE);
	RCC->CFGR0 |= RCC_HPRE_DIV128 | RCC_ADCPRE_DIV2;  //HBCLK = 187.5 kHz
	SysTick->CNT = 0;
	SysTick->CMP = TIMER_UPDATE;
}
}

int main(void)
{
	uint8_t tmp;
	NVIC_EnableIRQ(SysTick_IRQn);
	NVIC_EnableIRQ(USART1_IRQn);
	NVIC_EnableIRQ(EXTI7_0_IRQn);
	NVIC_EnableIRQ(HardFault_IRQn);
	
	__enable_irq();

	uart1.print("UART is work!!! \n");
	sens_val_border = flash_get(); 
	
	while(1) {
		switch(awtr_state) {
		case blink:
			led_on();
			delayT(MS_TO_TICKS(10));
			led_off();
			awtr_state = waiting;
			break;
		case waiting:
			uint32_t cnt_prev, cnt_cur;
			cnt_prev = SysTick->CNT;
			cnt_cur = SysTick->CNT;
			while(cnt_cur < cnt_prev + MS_TO_TICKS(4000) && awtr_state == waiting) {
				cnt_cur = SysTick->CNT;
			}
			break;

		case click:
			sens_val = sens_get();
			if(sens_val > sens_val_border) {
				__disable_irq();
				pump_on();
				delayT(MS_TO_TICKS(AWTR_WATERING_TIME));
				pump_off();
				__enable_irq();
				intToASCII(sens_val, msg_click_dry + 15, 3);
				uart1.send(msg_click_dry, sizeof(msg_click_dry) - 1);
			}
			else {
				intToASCII(sens_val, msg_click_wet + 15, 3);
				uart1.send(msg_click_wet, sizeof(msg_click_wet) - 1);
			}
			awtr_state = toSleep;
			break;
		case hold:
			sens_val_border = sens_get();
			tmp = flash_save(sens_val_border);
			sens_val_border = flash_get();
			led_on();
			intToASCII(tmp, msg_hold + 1, 1);
			intToASCII(sens_val_border, msg_hold + 17, 3);
			uart1.send(msg_hold, sizeof(msg_hold) - 1);
			delayT(MS_TO_TICKS(10));
			led_off();
			awtr_state = toSleep;
			break;
		case long_hold:
			uart1.print("Measurement cycle, click button for exit\n");
			while(awtr_state == long_hold) {
				sens_val = sens_get();
				intToASCII(sens_val, msg_lhold + 14, 3);
				uart1.send(msg_lhold, sizeof(msg_lhold) - 1);
				delayT(MS_TO_TICKS(1000));
			}
			break;

		case toSleep:
			// uart1.sync();
			break;
		}
	}
	return 0;
}

/* Interrupts */
extern "C"{
__attribute((interrupt("WCH-Interrupt-fast")))
void USART1_IRQHandler(void) {
    uart1.interruptHandler();
}

__attribute((interrupt("WCH-Interrupt-fast")))
void SysTick_IRQHandler(void) {
	SysTick->SR = 0;
	uart1.print("Timer update\n");
	awtr_state = blink;
	return;
}

__attribute((interrupt("WCH-Interrupt-fast")))
void EXTI7_0_IRQHandler(void) {
	EXTI->INTFR |= 1 << btn_pin;
	static uint32_t wait;

	static uint8_t btn_state_last = 0;
	uint8_t btn_state_new = btn_state();
	if(btn_state_new == btn_state_last) {
		return;
	}
	btn_state_last = btn_state_new;

	if(btn_state_new) {
		uart1.print("button was pressed\n");
		awtr_state = blink;
		wait = SysTick->CNT;
	}
	else {
		uart1.print("button was released\n");
		wait = SysTick->CNT - wait;
		if(wait < MS_TO_TICKS(10)) {
			awtr_state = toSleep;
		}
		else if(wait < MS_TO_TICKS(700)) {
			awtr_state = click;
		}
		else if(wait < MS_TO_TICKS(3000)) {
			awtr_state = hold;
		}
		else {
			awtr_state = long_hold;
		}
	}
	return;
}

__attribute((interrupt("WCH-Interrupt-fast")))
void HardFault_Handler(void) {
	led_on();
	while(1) {;}
}
}