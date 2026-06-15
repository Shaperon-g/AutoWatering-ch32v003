#ifndef GPIO_H_
#define GPIO_H_

#include <ch32v00x.h>

/* for initialization */
#define IO_MASK    0b1111

#define IO_IN      0b0000
#define IO_OUT_10M 0b0001
#define IO_OUT_2M  0b0010
#define IO_OUT_50M 0b0011

#define IO_OUT_PP  0b0000
#define IO_OUT_OD  0b0100
#define IO_OUT_APP 0b1000
#define IO_OUT_AOD 0b1100
#define IO_IN_AN   0b0000
#define IO_IN_FL   0b0100
#define IO_IN_PUD  0b1000

#define IO_P0 0
#define IO_P1 4
#define IO_P2 8
#define IO_P3 12
#define IO_P4 16
#define IO_P5 20
#define IO_P6 24
#define IO_P7 28

/* pinout */
#define LED_PORT GPIOC
#define LED_GPIN IO_P1
enum {led_pin = 1};

#define PUMP_PORT GPIOC
#define PUMP_GPIN IO_P2
enum {pump_pin = 2};

#define BTN_PORT GPIOC
#define BTN_GPIN IO_P4
enum {btn_pin = 4};
#define BTN_EXTI AFIO_EXTICR_EXTI4_PC

#define UTX_PORT GPIOD
#define UTX_GPIN IO_P6  //remap
enum {utx_pin = 6};
#define UTX_REMAP AFIO_PCFR1_USART1_REMAP_REMAP2

#define SENS_PORT GPIOA
#define SENS_GPIN IO_P2
enum {sens_pin = 2};

/* LED, pump and button */
void led_on();
void led_off();
void pump_on();
void pump_off();
uint8_t btn_state(); // 1 - pressed, 0 - released
uint16_t sens_get();

#endif