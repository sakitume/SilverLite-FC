#include "_my_config.h"
#ifdef RX_FLYSKY

#include "flash.h"
#include "main.h"

#include "drv_time.h"
#include "afhds2a_support.h"

//------------------------------------------------------------------------------
extern void rxSpiInit();    // rx_afhds2a/rx_spi.c
extern float vbattfilt;

//------------------------------------------------------------------------------
#define INTERVAL_RX_LOSS_MS 1000
#define INTERVAL_RX_BIND_MS 250
#define RX_LOSS_COUNT 1000

static flySkyConfig_t gflySkyConfig;
static bool bindRequested;
static bool lastBindPinStatus;

#if defined(MATEKF411RX)
static const bool ledInversion = true;  // MATEKF411RX defines RX_SPI_LED_INVERTED
#elif defined(CRAZYBEEF3FS)
static const bool ledInversion = true;  // CRAZYBEEF3FR does *not* define RX_SPI_LED_INVERTED, but they end up staying on during flight so turn it off by setting ledInversion to true
#endif

//------------------------------------------------------------------------------
const flySkyConfig_t* flySkyConfig(void)
{
    return &gflySkyConfig; 
}

//------------------------------------------------------------------------------
flySkyConfig_t* flySkyConfigMutable(void)
{ 
    return &gflySkyConfig;
}

//------------------------------------------------------------------------------
void writeEEPROM(void)
{
    __disable_irq();    // To (hopefully) prevent lockups that sometimes occur when saving to flash
    flash_save();
    __enable_irq();
}

//------------------------------------------------------------------------------
// afhds2a.c only checks if the return value of this function is non-zero
IO_t IOGetByTag(ioTag_t tag)     
{
    (void)tag;
    return (void*)1; 
}

//------------------------------------------------------------------------------
// Called by flySkyInit(), we use this opportunity to initialize our lower level
// SPI code. AND we also use this to check if we were able to retrieve a
// usable flySkyConfig_t from flash and if not, enter bind mode
void rxSpiCommonIOInit(const rxSpiConfig_t *rxSpiConfig)
{
    (void)rxSpiConfig;

    lastBindPinStatus = (RX_SPI_BIND_PIN_GPIO_Port->IDR & RX_SPI_BIND_PIN_Pin);

    rxSpiInit();

    if (gflySkyConfig.txId == 0)
    {
        bindRequested = true;
    }
}

//------------------------------------------------------------------------------
// Can be called by console to initiate a bind
void rxSpiBind(void)
{
    bindRequested = true;
}

//------------------------------------------------------------------------------
bool rxSpiCheckBindRequested(bool reset)
{ 
    // A bind is requested if bind pin state has toggled
    bool bindPinStatus = (RX_SPI_BIND_PIN_GPIO_Port->IDR & RX_SPI_BIND_PIN_Pin);
    if (lastBindPinStatus && !bindPinStatus) 
    {
        bindRequested = true;
    }
    lastBindPinStatus = bindPinStatus;

    if (!bindRequested) 
    {
        return false;
    } 
    else 
    {
        if (reset) 
        {
            bindRequested = false;
        }
        return true;
    }
}

//------------------------------------------------------------------------------
void rxSpiLedToggle(void)
{
    uint32_t mask = RX_SPI_LED_PIN_Pin;
    if (RX_SPI_LED_PIN_GPIO_Port->ODR & mask) 
    {
        RX_SPI_LED_PIN_GPIO_Port->BSRR = mask << 16;
    } 
    else 
    {
        RX_SPI_LED_PIN_GPIO_Port->BSRR = mask;
    }
}

//------------------------------------------------------------------------------
static void rxSpiLedBlink(uint32_t blinkMs)
{
    static uint32_t ledBlinkMs = 0;

    uint32_t now = gettime() / 1000;

    if ((ledBlinkMs + blinkMs) > now) {
        return;
    }
    ledBlinkMs = now;

    rxSpiLedToggle();
}

//------------------------------------------------------------------------------
static void rxSpiLedOn(void)
{
    if (ledInversion)
    {
        // Set pin Lo
        RX_SPI_LED_PIN_GPIO_Port->BSRR = RX_SPI_LED_PIN_Pin << 16;
    }
    else
    {
        // Set pin Hi
        RX_SPI_LED_PIN_GPIO_Port->BSRR = RX_SPI_LED_PIN_Pin;
    } 
}

//------------------------------------------------------------------------------
void rxSpiLedBlinkRxLoss(rx_spi_received_e result)
{
    static uint16_t rxLossCount = 0;

    if (result == RX_SPI_RECEIVED_DATA) 
    {
        rxLossCount = 0;
        rxSpiLedOn();
    } 
    else 
    {
        if (rxLossCount < RX_LOSS_COUNT) 
        {
            rxLossCount++;
        }
        else 
        {
            rxSpiLedBlink(INTERVAL_RX_LOSS_MS);
        }
    }
}

//------------------------------------------------------------------------------
void rxSpiLedBlinkBind(void)
{
    rxSpiLedBlink(INTERVAL_RX_BIND_MS);
}

//------------------------------------------------------------------------------
uint16_t getBatteryVoltage(void)
{
	return (vbattfilt * 100 + 0.5f);
}


#endif  // #ifdef RX_FLYSKY

