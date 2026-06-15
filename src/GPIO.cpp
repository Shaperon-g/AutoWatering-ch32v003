#include "GPIO.h"


void led_on(){
    LED_PORT->BCR = 1 << led_pin;
}
void led_off(){
    LED_PORT->BSHR = 1 << led_pin;
}
void pump_on(){
    PUMP_PORT->BSHR = 1 << pump_pin;
}
void pump_off(){
    PUMP_PORT->BCR = 1 << pump_pin;
}
uint8_t btn_state() { // 1 - pressed, 0 - not
    if(BTN_PORT->INDR & (1 << btn_pin)) {
        return 0;
    }
    return 1;
}

uint16_t sens_get() {
    return 100;
}