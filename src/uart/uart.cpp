#include "uart.h"

uart::uart(USART_TypeDef* cur_usart, int baudrate, int f_clk, u_stopbit mode_stopBit, 
    u_wordlen word_len)
{
    usart = cur_usart;

    usart->BRR = (uint16_t)(f_clk/baudrate);
    usart->CTLR1 = USART_CTLR1_UE | USART_CTLR1_RE | USART_CTLR1_TE | USART_CTLR1_RXNEIE;

    usart->CTLR2 |= mode_stopBit << 12;
    if(word_len == word_len_9)
        usart->CTLR1 |= USART_CTLR1_M;

    usart->CTLR3 = 0;
    usart->STATR = 0;
}
    
bool uart::send(uint8_t *data, uint16_t len, void (*callback)(void), uint8_t conf)
{
    //buffer is owerflow
    if((tx_posE + 1 == tx_posS) || (tx_posS == 0 && tx_posE == BUF_TX_LEN - 1))
        return false;
    bufferTX[tx_posE].data = data;
    bufferTX[tx_posE].len = len;
    bufferTX[tx_posE].cnt = 0;
    bufferTX[tx_posE].callback = callback;
    bufferTX[tx_posE].conf_l = conf;
    tx_posE ++;
    if(tx_posE >= BUF_TX_LEN)
        tx_posE = 0;
    usart->CTLR1 |= USART_CTLR1_TXEIE;
    return true;
}
bool uart::print(const char* str)
{
    int cnt = 0;
    while(str[cnt] != 0) {
        cnt ++;
    }
    return send((uint8_t*)str, cnt);
}


void uart::sync(void)
{
    while(!(usart->STATR & USART_STATR_TC) || tx_posS != tx_posE) {;}
    return;
}
    
int uart::available(void)
{
    if(rx_posS <= rx_posE)
        return rx_posE - rx_posS;
    return BUF_RX_LEN - rx_posS + rx_posE;
}
uint8_t uart::get(void)
{
    //buffer is empty
    if(rx_posS == rx_posE)
        return 0;
    uint8_t val = bufferRX[rx_posS];
    rx_posS ++;
    if(rx_posS >= BUF_RX_LEN)
        rx_posS = 0;
    return val;
}
void uart::interruptHandler(void)
{
    //Transmit
    if((usart->STATR & USART_STATR_TXE) && (usart->CTLR1 & USART_CTLR1_TXEIE)) {
        //buffer is empty
        if(tx_posS == tx_posE) {
            usart->CTLR1 &= ~(USART_CTLR1_TXEIE);
        }
        else {
            uint16_t tmp = 0;
            //9 bits
            if(bufferTX[tx_posS].conf_l & UART_CONF_9BIT) {
                tmp = *(uint16_t*)(bufferTX[tx_posS].data + bufferTX[tx_posS].cnt);
                bufferTX[tx_posS].cnt += 2;
            }
            else {
                tmp = *(uint8_t*)(bufferTX[tx_posS].data + bufferTX[tx_posS].cnt);
                bufferTX[tx_posS].cnt += 1;
            }
            usart->DATAR = tmp;
            if(bufferTX[tx_posS].cnt >= bufferTX[tx_posS].len) {
                if(bufferTX[tx_posS].callback != nullptr)
                    bufferTX[tx_posS].callback();
                tx_posS ++;
                if(tx_posS >= BUF_TX_LEN)
                    tx_posS = 0;
            }
        }
    }
    //Receieve
    if((usart->STATR & USART_STATR_RXNE) && (usart->CTLR1 & USART_CTLR1_RXNEIE)) {
        //9th bit
        if(usart->CTLR1 & USART_CTLR1_M) {
            //buffer is not overflow
            if(!((rx_posS == 0 && rx_posE == BUF_RX_LEN - 1) || (rx_posE + 1 == rx_posS))) {
                bufferRX[rx_posE] = usart->DATAR >> 8;
                rx_posE ++;
                if(rx_posE >= BUF_RX_LEN)
                    rx_posE = 0;
                }
        }
        //8 bits
        if(!((rx_posS == 0 && rx_posE == BUF_RX_LEN - 1) || (rx_posE + 1 == rx_posS))) {
            bufferRX[rx_posE] = usart->DATAR & 0x00FF;
            rx_posE ++;
            if(rx_posE >= BUF_RX_LEN)
                rx_posE = 0;
        }
    }
    usart->STATR = 0;
    return;
}