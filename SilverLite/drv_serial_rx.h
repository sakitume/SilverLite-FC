#ifndef _DRV_SERIAL_RX_H_
#define  _DRV_SERIAL_RX_H_

void Serial_RX_Init();

typedef void (*SerialIRQHandler_t)(void* usart);
void Serial_RX_SetRxIRQHandler(SerialIRQHandler_t handler);
void Serial_RX_SetTxIRQHandler(SerialIRQHandler_t handler);

#endif

