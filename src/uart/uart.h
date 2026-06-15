#ifndef UART_H_
#define UART_H_

#include <ch32v00x.h>


#define BUF_TX_LEN 10 //letters
#define BUF_RX_LEN 50 //bytes

/* 9bit data:  0x01FF01FF01FF...  other bits don't work*/
#define UART_CONF_8BIT 0b0000
#define UART_CONF_9BIT 0b0001
struct uart_letter {
    uint8_t conf_l;
    uint8_t *data;
    uint16_t len;
    uint16_t cnt;
    void (*callback)(void);
};

enum u_stopbit {
    stop_0d5_bit = 1,
    stop_1_bit = 0,
    stop_1d5_bit = 3,
    stop_2_bit = 2
};
enum u_wordlen {
    word_len_8 = 0,
    word_len_9 = 1
};

class uart {
    uart_letter bufferTX[BUF_TX_LEN];
    uint8_t bufferRX[BUF_RX_LEN];
    volatile uint8_t tx_posS = 0, tx_posE = 0, rx_posS = 0, rx_posE = 0;
    USART_TypeDef * usart;

public:

    uart(USART_TypeDef* cur_usart, int baudrate, int f_clk, u_stopbit mode_stopBit = stop_1_bit, 
        u_wordlen word_len = word_len_8);

    bool send(uint8_t *data, uint16_t len = 0, void (*callback)(void) = nullptr, uint8_t conf = 0);
    bool print(const char* str);
    void sync(void);

    int available(void);
    uint8_t get(void);
    
    void interruptHandler(void);
};

#endif