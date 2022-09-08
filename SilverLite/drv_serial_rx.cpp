#include "_my_config.h"
#include "drv_serial_rx.h"
#include <stdint.h>

#if defined(RX_ELRS)
    #if !defined(SERIAL_RX)
        #define SERIAL_RX
    #endif
#endif

#if defined(SERIAL_RX)

#if defined(STM32F4)
    #include "stm32f4xx.h"
    #include "stm32f4xx_ll_gpio.h"
    #include "stm32f4xx_ll_rcc.h"
    #include "stm32f4xx_ll_bus.h"
    #include "stm32f4xx_ll_usart.h"
#endif

#if defined(STM32F3)
    #include "stm32f3xx.h"
    #include "stm32f3xx_ll_gpio.h"
    #include "stm32f3xx_ll_bus.h"
    #include "stm32f3xx_ll_usart.h"
#endif

//------------------------------------------------------------------------------
#define GPIO_AF_USART1  GPIO_AF7_USART1
#define GPIO_AF_USART2  GPIO_AF7_USART2
#define GPIO_AF_USART3  GPIO_AF7_USART3
#define GPIO_AF_USART4  GPIO_AF8_UART4
#define GPIO_AF_USART5  GPIO_AF8_UART5
#define GPIO_AF_USART6  GPIO_AF8_USART6
#define GPIO_AF_USART7  GPIO_AF8_UART7
#define GPIO_AF_USART8  GPIO_AF8_UART8

#define ENABLE_CLOCK_USART1     LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_USART1)
#define ENABLE_CLOCK_USART2     LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_USART2)
#define ENABLE_CLOCK_USART3     LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_USART3)
#define ENABLE_CLOCK_USART4     LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_UART4)
#define ENABLE_CLOCK_USART5     LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_UART5)
#define ENABLE_CLOCK_USART6     LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_USART6)

#define ENABLE_IRQ_USART1       USART_EnableIRQ(USART1_IRQn, 0, 0)
#define ENABLE_IRQ_USART2       USART_EnableIRQ(USART2_IRQn, 0, 0)
#define ENABLE_IRQ_USART3       USART_EnableIRQ(USART3_IRQn, 0, 0)
#define ENABLE_IRQ_USART4       USART_EnableIRQ(UART4_IRQn, 0, 0)
#define ENABLE_IRQ_USART5       USART_EnableIRQ(UART5_IRQn, 0, 0)
#define ENABLE_IRQ_USART6       USART_EnableIRQ(USART6_IRQn, 0, 0)
static void USART_EnableIRQ(IRQn_Type irq, uint32_t preemptPriority, uint32_t subPriority)
{
    NVIC_SetPriorityGrouping(2);
    NVIC_SetPriority(irq, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), preemptPriority, subPriority));
    NVIC_EnableIRQ(irq);
}

#if 0
#define DISABLE_IRQ_USART1      USART_DisableIRQ(USART1_IRQn)
#define DISABLE_IRQ_USART2      USART_DisableIRQ(USART2_IRQn)
#define DISABLE_IRQ_USART3      USART_DisableIRQ(USART3_IRQn)
#define DISABLE_IRQ_USART4      USART_DisableIRQ(UART4_IRQn)
#define DISABLE_IRQ_USART5      USART_DisableIRQ(UART5_IRQn)
#define DISABLE_IRQ_USART6      USART_DisableIRQ(USART6_IRQn)
static void USART_DisableIRQ(IRQn_Type irq)
{
    NVIC_DisableIRQ(irq);
}
#endif

//------------------------------------------------------------------------------
static void USART_IRQHandler(USART_TypeDef* usart);
#define USART_IRQ_HANDLER(channel) \
    extern "C" void USART##channel##_IRQHandler() { USART_IRQHandler(USART); } \
    extern "C" void UART##channel##_IRQHandler()  { USART_IRQHandler(USART); }

//------------------------------------------------------------------------------
// NOX  - Uses USART2, disable inverter (PC14)
#if defined(NOX)
    #define NOX_USART2
    #if defined(NOX_USART2)
    #define USART_INVERTER_PIN      LL_GPIO_PIN_14
    #define USART_INVERTER_PORT     GPIOC
    #define USART                   USART2
    #define USART_RX_PIN            LL_GPIO_PIN_3       // PA3
    #define USART_TX_PIN            LL_GPIO_PIN_2       // PA2
    #define USART_RX_TX_PORT        GPIOA
    #define USART_AF                GPIO_AF_USART2
    #define USART_ENABLE_CLOCK      ENABLE_CLOCK_USART2
    #define USART_ENABLE_IRQ        ENABLE_IRQ_USART2
    USART_IRQ_HANDLER(2);
    #else
    #define USART                   USART1
    #define USART_RX_PIN            LL_GPIO_PIN_7       // PB7 (R1 on center of board, backside. T1 on top/right center of board, topside)
    #define USART_TX_PIN            LL_GPIO_PIN_6       // PB6
    #define USART_RX_TX_PORT        GPIOB
    #define USART_AF                GPIO_AF_USART1
    #define USART_ENABLE_CLOCK      ENABLE_CLOCK_USART1
    #define USART_ENABLE_IRQ        ENABLE_IRQ_USART1
    USART_IRQ_HANDLER(1);
    #endif
#endif

//------------------------------------------------------------------------------
// OMINBUSF4  - Use USART1, disable inverter (PC0)
#if defined(OMNIBUSF4)
    #define USART_INVERTER_PIN      LL_GPIO_PIN_0
    #define USART_INVERTER_PORT     GPIOC
    #define USART                   USART1
    #define USART_RX_PIN            LL_GPIO_PIN_10      // PA10
    #define USART_TX_PIN            LL_GPIO_PIN_9       // PA9
    #define USART_RX_TX_PORT        GPIOA
    #define USART_AF                GPIO_AF_USART1
    #define USART_ENABLE_CLOCK      ENABLE_CLOCK_USART1
    #define USART_ENABLE_IRQ        ENABLE_IRQ_USART1
    USART_IRQ_HANDLER(1);
#endif

//------------------------------------------------------------------------------
// OMINBUS  - Use USART3
#if defined(OMNIBUS)
    #define USART                   USART3
    #define USART_RX_PIN            LL_GPIO_PIN_11      // PB11
    #define USART_TX_PIN            LL_GPIO_PIN_10      // PB10
    #define USART_RX_TX_PORT        GPIOB
    #define USART_AF                GPIO_AF_USART3
    #define USART_ENABLE_CLOCK      ENABLE_CLOCK_USART3
    #define USART_ENABLE_IRQ        ENABLE_IRQ_USART3
    USART_IRQ_HANDLER(3);
#endif

//------------------------------------------------------------------------------
// CRAZYBEEF3FS  - Use USART3
#if defined(CRAZYBEEF3FS)
    #define USART                   USART3
    #define USART_RX_PIN            LL_GPIO_PIN_11      // PB11
    #define USART_TX_PIN            LL_GPIO_PIN_10      // PB10
    #define USART_RX_TX_PORT        GPIOB
    #define USART_AF                GPIO_AF_USART3
    #define USART_ENABLE_CLOCK      ENABLE_CLOCK_USART3
    #define USART_ENABLE_IRQ        ENABLE_IRQ_USART3
    USART_IRQ_HANDLER(3);
#endif

//------------------------------------------------------------------------------
enum class ESerialRX : uint8_t
{
    kIBUS,
    kELRS2
};

//------------------------------------------------------------------------------
void Serial_RX_Init()
{
#if defined(RX_IBUS)
    ESerialRX serialRxMode = ESerialRX::kIBUS;
#endif
#if defined(RX_ELRS)
    ESerialRX serialRxMode = ESerialRX::kELRS2;
#endif

    LL_GPIO_InitTypeDef gpio_init;
    gpio_init.Mode  = LL_GPIO_MODE_ALTERNATE;
    gpio_init.Speed = LL_GPIO_SPEED_FREQ_HIGH;

    // Configure RX pin, note: We want it to have a pullup and be open drain
    gpio_init.OutputType    = LL_GPIO_OUTPUT_OPENDRAIN;
    gpio_init.Pull          = LL_GPIO_PULL_UP;
    gpio_init.Pin           = USART_RX_PIN;
    gpio_init.Alternate     = USART_AF;
    LL_GPIO_Init(USART_RX_TX_PORT, &gpio_init);

    // Configure TX pin, for this we want push/pull and no pullup/pulldown
    gpio_init.OutputType    = LL_GPIO_OUTPUT_PUSHPULL;
    gpio_init.Pull          = LL_GPIO_PULL_NO;
    gpio_init.Pin           = USART_TX_PIN;
    gpio_init.Alternate     = USART_AF;
    LL_GPIO_Init(USART_RX_TX_PORT, &gpio_init);

    USART_ENABLE_CLOCK;

    LL_USART_Disable(USART);
    LL_USART_DeInit(USART);

    LL_USART_InitTypeDef usart_init;
    LL_USART_StructInit(&usart_init);
    usart_init.HardwareFlowControl  = LL_USART_HWCONTROL_NONE;
    usart_init.DataWidth            = LL_USART_DATAWIDTH_8B;
    usart_init.OverSampling         = LL_USART_OVERSAMPLING_16;
    usart_init.StopBits             = LL_USART_STOPBITS_1;
    usart_init.Parity               = LL_USART_PARITY_NONE;
    switch (serialRxMode)
    {
        case ESerialRX::kIBUS:
        {
            usart_init.BaudRate = 115200;
            usart_init.TransferDirection = LL_USART_DIRECTION_RX;
        }
        break;

        case ESerialRX::kELRS2:
        {
            usart_init.BaudRate = 420000;
            usart_init.TransferDirection = LL_USART_DIRECTION_TX_RX;
        }
        break;

        default:
            break;
    }

    LL_USART_Init(USART, &usart_init);

    // Disable inverter (if there is one)
#if defined(USART_INVERTER_PIN) && defined(USART_INVERTER_PORT)    
    gpio_init.Mode          = LL_GPIO_MODE_OUTPUT;
    gpio_init.OutputType    = LL_GPIO_OUTPUT_PUSHPULL;
    gpio_init.Pull          = LL_GPIO_PULL_NO;
    gpio_init.Pin           = USART_INVERTER_PIN;
    LL_GPIO_Init(USART_INVERTER_PORT, &gpio_init);
    LL_GPIO_ResetOutputPin(USART_INVERTER_PORT, USART_INVERTER_PIN);
#endif    

    LL_USART_DisableHalfDuplex(USART);
    LL_USART_ConfigAsyncMode(USART);

    LL_USART_Enable(USART);

    USART_ENABLE_IRQ;
    LL_USART_EnableIT_RXNE(USART);
}

//------------------------------------------------------------------------------
static SerialIRQHandler_t   gRxIRQHandler;
static SerialIRQHandler_t   gTxIRQHandler;
static void USART_IRQHandler(USART_TypeDef* usart)
{
    if (LL_USART_IsEnabledIT_TC(USART) && LL_USART_IsActiveFlag_TC(USART))
    {
        LL_USART_ClearFlag_TC(USART);
        if (gTxIRQHandler)
        {
            gTxIRQHandler(usart);
        }
    }
    else
    {
        if (gRxIRQHandler)
        {
            gRxIRQHandler(usart);
        }
    }
}

//------------------------------------------------------------------------------
void Serial_RX_SetRxIRQHandler(SerialIRQHandler_t handler)
{
    gRxIRQHandler = handler;
}

//------------------------------------------------------------------------------
void Serial_RX_SetTxIRQHandler(SerialIRQHandler_t handler)
{
    gTxIRQHandler = handler;
}

#endif
