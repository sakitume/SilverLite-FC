#ifndef __TRX_H__
#define __TRX_H__

#include <stdint.h>
#include <stdbool.h>

//------------------------------------------------------------------------------
// Enable only one of these definitions
#define TRX_NRF
//#define TRX_XN297
//#define TRX_XN297L
//#define TRX_LT8900

#ifdef __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------
extern void trx_Init();
extern void trx_SetAddr(uint8_t addr[]);
extern void trx_SetChannel(uint8_t channel);
extern bool trx_TelemetrySent();
extern uint8_t trx_PacketAvailable();
extern void trx_FlushTX();
extern void trx_FlushRX();
extern void trx_SetToRXMode();
extern void trx_PrepareToSendTelemetry();
extern void trx_SendTelemetryPacket(int data[], int size);
extern bool trx_ReadPayload(int data[], int size);

//------------------------------------------------------------------------------
extern void trx_spi_write(int data);
extern int trx_spi_read();
extern void trx_spi_init();
extern void trx_spi_cs_enable();
extern void trx_spi_cs_disable();

extern uint32_t gettime();
extern int xprintf(const char* fmt, ...);

#ifdef __cplusplus
}
#endif


//------------------------------------------------------------------------------
extern void delay_ms(uint32_t ms);
extern void delay_us(uint32_t us);

#if defined(__MODULE_DEV__) || defined(ARDUINO)
	#define XPRINTF  xprintf
#else
	#define XPRINTF(fmt, ...)
#endif

#endif 
