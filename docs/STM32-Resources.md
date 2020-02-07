SystemClock
ADC1                - Battery voltage
TIM1
    NOX
        TIM1_UP:    DMA2, Stream 5, NVIC global interrupt enabled
        TIM1_CH:    DMA2, Stream 1, NVIC global interrupt enabled
        TIM1_CH2:   DMA2, Stream 2, NVIC global interrupt enabled
    OMNIBUS
        TIM1_UP:    DMA2, Stream 5, NVIC global interrupt enabled
        TIM1_CH:    DMA2, Stream 1, NVIC global interrupt enabled
        TIM1_CH2:   DMA2, Stream 2, NVIC global interrupt enabled

TIM2 for gettime()

Blackbox
    NOX:        USART2, 2MB, 8N1  
        USART2_TX: DMA1, Stream 6
    OMNIBUS:    UART4, 2MB, 8N1
        UART4_TX: DMA1, Stream 4

DSHOT (drv_dshot_bdir and drv_dshot_dma) uses TIM1, DMA2