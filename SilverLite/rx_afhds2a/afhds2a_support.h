#include "_my_config.h"
#if defined(RX_FLYSKY) || defined(RX_FLYSKY2A)

#include "afhds2a.h"

#ifdef __cplusplus
extern "C" {
#endif

// External references
extern IO_t IOGetByTag(ioTag_t tag);
extern void rxSpiCommonIOInit(const rxSpiConfig_t *rxSpiConfig);
extern bool rxSpiCheckBindRequested(bool reset);
extern void rxSpiLedBlinkRxLoss(rx_spi_received_e result);
extern void rxSpiLedBlinkBind(void);
extern uint16_t getBatteryVoltage(void);
extern void writeEEPROM(void);


#ifdef __cplusplus
}
#endif

#endif  // #if defined(RX_FLYSKY) || defined(RX_FLYSKY2A)
