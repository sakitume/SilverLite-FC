#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void console_poll(void);
void console_init(void);


#define CONSOLE_PACKET_BUFFER_MAX   256
extern uint8_t console_packet_buffer[];
void console_sendPacket(uint8_t cmdID, uint8_t *payload, uint8_t payloadLen);

void console_openPacket(void);
void console_closePacket(uint8_t cmdID);
void console_appendPacket8(uint8_t b);
void console_appendPacket16(uint16_t b);

int xprintf(const char* fmt, ...);



#ifdef __cplusplus
}
#endif