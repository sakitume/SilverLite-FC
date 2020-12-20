// Some useful info:
// https://github.com/mbriday/ExampleSTM32F303
// 

namespace Periph { 
#if 0
    constexpr uint32_t rtc   = 0x40002800;
    constexpr uint32_t pwr   = 0x40007000;
#endif    
    constexpr uint32_t gpio  = 0x48000000;      // pg 54, GPIOA
    constexpr uint32_t rcc   = 0x40021000;      // pg 55, 137, RCC
#if 0    
    constexpr uint32_t flash = 0x40023C00;
    constexpr uint32_t fsmc  = 0xA0000000;
#endif

    inline volatile uint32_t& bit (uint32_t a, int b) {
        return MMIO32(0x42000000 + ((a & 0xFFFFF) << 5) + (b << 2));
    }
}

// interrupt vector table in ram
struct VTable {
    typedef void (*Handler)();

    uint32_t* initial_sp_value;
    Handler
        reset, 
        nmi, 
        hard_fault,
        memory_manage_fault, 
        bus_fault, 
        usage_fault,
        dummy_x001c[4], 
        sv_call, 
        debug_monitor, 
        dummy_x0034, 
        pend_sv, 
        systick;
    // pg 286-288 RM0316, Table 82. STM32F303xB/C/D/E, STM32F358xC and STM32F398xE vector table
    Handler
        wwdg,           // 0    0x40
        pvd,            // 1    0x44
        tamp_stamp,     // 2    0x48
        rtc_wkup,       // 3
        flash,          // 4
        rcc,            // 5
        exti0,          // 6
        exti1,          // 7
        exti2,          // 8
        exti3,          // 9
        exti4,          // 10
        dma1_stream0,   // 11   DMA1_Channel1
        dma1_stream1,   // 12
        dma1_stream2,   // 13
        dma1_stream3,   // 14
        dma1_stream4,   // 15
        dma1_stream5,   // 16
        dma1_stream6,   // 17
        adc,            // 18   ADC1_2
        can1_tx,        // 19   CAN_TX
        can1_rx0,       // 20
        can1_rx1,       // 21
        can1_sce,       // 22
        exti9_5,        // 23
        tim1_brk_tim15, // 24
        tim1_up_tim16,  // 25
        tim1_trg_com_tim17, 
        tim1_cc,        // 27
        tim2,           // 28
        tim3,           // 29
        tim4,           // 30
        i2c1_ev,        // 31
        i2c1_er,        // 32
        i2c2_ev,        // 33
        i2c2_er,        // 34
        spi1,           // 35
        spi2,           // 36
        usart1,         // 37
        usart2,         // 38
        usart3,         // 39
        exti15_10,      // 40
        rtc_alarm,      // 41
        USBWakeUp,      // 42
        TIM8_BRK,       // 43
        TIM8_UP,        // 44
        TIM8_TRG_COM,   // 45
        TIM8_CC,        // 46
        ADC3,           // 47
        dummy_48,       // 48
        dummy_49,       // 49
        dummy_50,       // 50
        spi3,           // 51
        uart4,          // 52
        uart5,          // 53
        tim6_dac,       // 54
        tim7,           // 55
        dma2_stream0,   // 56   DMA2_Channel1
        dma2_stream1,   // 57
        dma2_stream2,   // 58
        dma2_stream3,   // 59
        dma2_stream4,   // 60
        ADC4,           // 61
        dummy_62,       // 62
        dummy_63,       // 63
        COMP1_2_3,      // 64
        COMP4_5_6,      // 65
        COMP7,          // 66
        dummy_67,       // 67
        dummy_68,       // 68
        dummy_69,       // 69
        dummy_70,       // 70
        dummy_71,       // 71
        dummy_72,       // 72
        dummy_73,       // 73
        USB_HP,         // 74
        USB_LP,         // 75
        USB_WakeUp_RMP, // 76
        dummy_77,       // 77
        dummy_78,       // 78
        dummy_79,       // 79
        dummy_80,       // 80
        FPU,            // 81
        dummy_82,       // 82
        dummy_83,       // 83
        dummy_84;       // 84
};


// gpio

enum class Pinmode {
    // mode (2), typer (1), pupdr (2)
    in_analog         = 0b0011000,
    in_float          = 0b0000000,
    in_pulldown       = 0b0000010,
    in_pullup         = 0b0000001,

    out               = 0b0101000,
    out_od            = 0b0101100,
    alt_out           = 0b0110000,
    alt_out_od        = 0b0110100,

    out_2mhz          = 0b0001000,
    out_od_2mhz       = 0b0001100,
    alt_out_2mhz      = 0b0010000,
    alt_out_od_2mhz   = 0b0010100,

    out_50mhz         = 0b1001000,
    out_od_50mhz      = 0b1001100,
    alt_out_50mhz     = 0b1010000,
    alt_out_od_50mhz  = 0b1010100,

    out_100mhz        = 0b1101000,
    out_od_100mhz     = 0b1101100,
    alt_out_100mhz    = 0b1110000,
    alt_out_od_100mhz = 0b1110100,
};

template<char port>
struct Port {
    constexpr static uint32_t base    = Periph::gpio + 0x400*(port-'A');
    constexpr static uint32_t moder   = base + 0x00;
    constexpr static uint32_t typer   = base + 0x04;
    constexpr static uint32_t ospeedr = base + 0x08;
    constexpr static uint32_t pupdr   = base + 0x0C;
    constexpr static uint32_t idr     = base + 0x10;
    constexpr static uint32_t odr     = base + 0x14;
    constexpr static uint32_t bsrr    = base + 0x18;
    constexpr static uint32_t afrl    = base + 0x20;
    constexpr static uint32_t afrh    = base + 0x24;

    static void mode (int pin, Pinmode m, int alt =0) {
        // enable GPIOx clock
        Periph::bit(Periph::rcc+0x14, port-'A' + 17) = 1;   // +17 because IOPAEN is at bit 17, pg149 RM0316

        // set the alternate mode before switching to it
        uint32_t afr = pin & 8 ? afrh : afrl;
        int shift = 4 * (pin & 7);
        MMIO32(afr) = (MMIO32(afr) & ~(0xF << shift)) | (alt << shift);

        int p2 = 2*pin;
        auto mval = static_cast<int>(m);
        MMIO32(ospeedr) = (MMIO32(ospeedr) & ~(3<<p2)) | (((mval>>5)&3) << p2);
        MMIO32(moder) = (MMIO32(moder) & ~(3<<p2)) | (((mval>>3)&3) << p2);
        MMIO32(typer) = (MMIO32(typer) & ~(1<<pin)) | (((mval>>2)&1) << pin);
        MMIO32(pupdr) = (MMIO32(pupdr) & ~(3<<p2)) | ((mval&3) << p2);
    }

    static void modeMap (uint16_t pins, Pinmode m, int alt =0) {
        for (int i = 0; i < 16; ++i) {
            if (pins & 1)
                mode(i, m, alt);
            pins >>= 1;
        }
    }
};

template<char port,int pin>
struct Pin {
    typedef Port<port> gpio;
    constexpr static uint16_t mask = 1U << pin;
    constexpr static int id = 16 * (port-'A') + pin;

    static void mode (Pinmode m, int alt =0) {
        gpio::mode(pin, m, alt);
    }

    static int read () {
        return mask & MMIO32(gpio::idr) ? 1 : 0;
    }

    static void write (int v) {
        MMIO32(gpio::bsrr) = v ? mask : mask << 16;
    }

    // shorthand
    operator int () const { return read(); }
    void operator= (int v) const { write(v); }

    // Warning, I've found through testing that this function fails to toggle
    // if you've never performed a write(1) before calling this. I've also found
    // that you can also get it into a state where toggle() stops working, maybe
    // based on whether you called write(0) before calling toggle().
    static void toggle () {
        // both versions below are non-atomic, they access and set in two steps
        // this is smaller and faster (1.6 vs 1.2 MHz on F103 @ 72 MHz):
        //MMIO32(gpio::odr) ^= mask;
        // but this code is safer, because it can't interfere with nearby pins:
        MMIO32(gpio::bsrr) = mask & MMIO32(gpio::odr) ? mask << 16 : mask;
    }
};


// u(s)art

template< typename TX, typename RX >
struct UartDev {
    // TODO does not recognise alternate TX pins
    // Does not recognize UART4, UART5 (that some F3 devices provide)
    // Construct uidx (0 based usart index), 0==USART1, 1==USART2, 2==USART3
    //                                      Port-+         +-Pin
    //                                           |         |
    constexpr static int uidx = TX::id == 16 * ('A'-'A') + 2 ? 1 :  // PA2, USART2
                                TX::id == 16 * ('A'-'A') + 9 ? 0 :  // PA9, USART1
                                TX::id == 16 * ('B'-'A') + 6 ? 0 :  // PB6, USART1
                                TX::id == 16 * ('B'-'A') + 10 ? 2 : // PB10, USART3
                                // TODO more possible, using alt mode 8 instead of 7
                                               0;   // else USART1


    constexpr static uint32_t base = uidx == 0 ? 0x40013800 :   // USART1, pg 52 peripheral register boundary addresses
                                     uidx == 1 ? 0x40004400 :   // USART2, pg 56 peripheral register boundary addresses
                                                 0x40004800;    // USART3, pg 56 peripheral register boundary addresses

    // pg 950 Table 165 USART register map   
    constexpr static uint32_t USART_ISR = base + 0x1C;
    constexpr static uint32_t USART_RDR = base + 0x24;
    constexpr static uint32_t USART_TDR = base + 0x28;
    constexpr static uint32_t USART_BRR = base + 0x0C;    // pg 950 and 940
    constexpr static uint32_t USART_CR1 = base + 0x00;    // pg 950 and 929

    static void init () {
        tx.mode(Pinmode::alt_out, 7);
        rx.mode(Pinmode::alt_out, 7);

        if (uidx == 0)
            Periph::bit(Periph::rcc+0x18, 14) = 1;      // enable USART1 clock (0x18 == RCC_APB2ENR, 14 == USART1EN)
        else
            Periph::bit(Periph::rcc+0x1C, 16+uidx) = 1; // enable U(S)ART 2..5 clock (0x40 = RCC_APB1ENR see pg 166, 152, bit 17 = USART2, bit 18 == USART3, etc)

        baud(115200);
        MMIO32(USART_CR1) = (1<<0) | (1<<3) | (1<<2);  // UE, TE, RE
    }

    static void baud (uint32_t baud, uint32_t hz = 72000000) {  // 72Mhz is default for F3
        MMIO32(USART_BRR) = (hz + baud/2) / baud;
    }

    static bool writable () {
        return (MMIO32(USART_ISR) & (1<<7)) != 0;  // TXE
    }

    static void putc (int c) {
        while (!writable()) {}
        MMIO32(USART_TDR) = (uint8_t) c;
    }

    static bool readable () {
        return (MMIO32(USART_ISR) & ((1<<5) | (1<<3))) != 0;  // RXNE or ORE
    }

    static int getc () {
        while (!readable()) {}
        return MMIO32(USART_RDR);
    }

    static TX tx;
    static RX rx;
};

template< typename TX, typename RX >
TX UartDev<TX,RX>::tx;

template< typename TX, typename RX >
RX UartDev<TX,RX>::rx;

// interrupt-enabled uart, sits on top of polled uart

template< typename TX, typename RX, int N =50 >
struct UartBufDev : UartDev<TX,RX> {
    typedef UartDev<TX,RX> base;

    static void init () {
        UartDev<TX,RX>::init();

        auto handler = []() {
            if (base::readable()) {
                int c = base::getc();
                if (recv.free())
                    recv.put(c);
                // else discard the input
            }
            if (base::writable()) {
                if (xmit.avail() > 0)
                    base::putc(xmit.get());
                else
                    Periph::bit(base::USART_CR1, 7) = 0;  // disable TXEIE
            }
        };

        switch (base::uidx) {
            case 0: VTableRam().usart1 = handler; break;
            case 1: VTableRam().usart2 = handler; break;
            case 2: VTableRam().usart3 = handler; break;
            case 3: VTableRam().uart4  = handler; break;
            case 4: VTableRam().uart5  = handler; break;
        }

        // nvic interrupt numbers are 37, 38, 39, 52, and 53, respectively
        constexpr uint32_t nvic_en1r = 0xE000E104;
        constexpr int irq = (base::uidx < 3 ? 37 : 49) + base::uidx;
        MMIO32(nvic_en1r) = 1 << (irq-32);  // enable USART interrupt

        Periph::bit(base::USART_CR1, 5) = 1;  // enable RXNEIE
    }

    static bool writable () {
        return xmit.free();
    }

    static void putc (int c) {
        while (!writable()) {}
        xmit.put(c);
        Periph::bit(base::USART_CR1, 7) = 1;  // enable TXEIE
    }

    static bool readable () {
        return recv.avail() > 0;
    }

    static int getc () {
        while (!readable()) {}
        return recv.get();
    }

    static RingBuffer<N> recv;
    static RingBuffer<N> xmit;
};

template< typename TX, typename RX, int N >
RingBuffer<N> UartBufDev<TX,RX,N>::recv;

template< typename TX, typename RX, int N >
RingBuffer<N> UartBufDev<TX,RX,N>::xmit;

