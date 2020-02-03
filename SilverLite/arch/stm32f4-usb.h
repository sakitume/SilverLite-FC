// Lean-and-mean USB serial driver for STM32F4xx

#ifndef debugf
#define debugf(...)
#endif

class UsbDev {
    constexpr static uint32_t base      = 0x50000000;
    constexpr static uint32_t GAHBCFG   = base + 0x008;  // p.1275
    constexpr static uint32_t GUSBCFG   = base + 0x00C;  // p.1276
    constexpr static uint32_t GINTSTS   = base + 0x014;  // p.1280
    constexpr static uint32_t GRXSTSP   = base + 0x020;  // p.1287
    constexpr static uint32_t GRXFSIZ   = base + 0x024;  // p.1288
    constexpr static uint32_t DIEPTXF0  = base + 0x028;  // p.1289
    constexpr static uint32_t GCCFG     = base + 0x038;  // p.1290
    constexpr static uint32_t DIEPTXF1  = base + 0x104;  // p.1292
    constexpr static uint32_t DCFG      = base + 0x800;  // p.1303
    constexpr static uint32_t DSTS      = base + 0x808;  // p.1305
    constexpr static uint32_t DIEPCTL0  = base + 0x900;  // p.1310
    constexpr static uint32_t DIEPTSIZ0 = base + 0x910;  // p.1321
    constexpr static uint32_t DTXFSTS0  = base + 0x918;  // p.1325
    constexpr static uint32_t DOEPCTL0  = base + 0xB00;  // p.1316
    constexpr static uint32_t DOEPTSIZ0 = base + 0xB10;  // p.1323

    static uint32_t inPending, inReady, inData;
    static bool dtr;  // only true when there is an active serial session

    static union Setup {
        struct { uint8_t typ, req; uint16_t val, idx, len; };
        uint32_t buf [2];
    } setupPkt;

    static const uint8_t devDesc [18];
    static const uint8_t cnfDesc [67];

    static volatile uint32_t& fifo (int ep) {
        return MMIO32(base + (ep+1) * 0x1000);
    }

    static void sendEp0 (void const* ptr, uint32_t len) {
        if (len > setupPkt.len)
            len = setupPkt.len;

        MMIO32(DIEPTSIZ0) = len;
        MMIO32(DIEPCTL0) |= (1<<31) | (1<<26);  // EPENA, CNAK

        uint32_t const* wptr = (uint32_t const*) ptr;
        for (uint32_t i = 0; i < len; i += 4)
            fifo(0) = *wptr++;
    }

    static void setConfig () {
        MMIO32(DOEPTSIZ0+0x20) = 64;  // accept 64b on RX ep1
        MMIO32(DOEPCTL0+0x20) = (3<<18) | (1<<15) | 64  // BULK ep1
                              | (1<<31) | (1<<26);      // EPENA, CNAK

        MMIO32(DOEPTSIZ0+0x40) = 64;  // accept 64b on RX ep2
        MMIO32(DOEPCTL0+0x40) = (2<<18) | (1<<15) | 64  // INTR ep2
                              | (1<<31) | (1<<26);      // EPENA, CNAK
    }

public:
    static void init () {
        MMIO32(Periph::rcc+0x34) |= (1<<7);  // OTGFSEN

        PinA<12> usbPin;
        usbPin.mode(Pinmode::out_od);
        delay_us(3000); // don't use delay_ms() because that calls console.poll()

        Port<'A'>::modeMap(0b0001100000000000, Pinmode::alt_out, 10);

        MMIO32(GCCFG) |= (1<<21) | (1<<16);  // NOVBUSSENS, PWRDWN
        MMIO32(GUSBCFG) |= (1<<30);  // FDMOD
        MMIO32(DCFG) |= (3<<0);  // DSPD
    }

    static void poll () {
        uint32_t irq = MMIO32(GINTSTS);
        //if (irq & ~0x04008028)
        //    debugf("irq %08x\n", irq);
        MMIO32(GINTSTS) = irq;  // clear all interrupts

        if (irq & (1<<13)) {  // ENUMDNE
            debugf("enumdne\n");

            MMIO32(GRXFSIZ)  = 512/4;                   // 512b for RX all
            MMIO32(DIEPTXF0) = (128/4<<16) | 512;       // 128b for TX ep0
            MMIO32(DIEPTXF1) = (512/4<<16) | (512+128); // 512b for TX ep1

            // see p.1354
            MMIO32(DIEPCTL0+0x20) = (1<<22)| (2<<18) | (1<<15) | 64 // fifo1
                                  | (1<<31) | (1<<26);  // EPENA, CNAK
            MMIO32(DIEPCTL0+0x40) = (2<<22)| (3<<18) | (1<<15) | 64; // fifo2

            MMIO32(DOEPTSIZ0) = (3<<29) | 64;               // STUPCNT, XFRSIZ
            MMIO32(DOEPCTL0) = (1<<31) | (1<<15) | (1<<26); // EPENA, CNAK
        }

        if ((irq & (1<<4)) && inPending == 0) {
            int rx = MMIO32(GRXSTSP), typ = (rx>>17) & 0xF,
                ep = rx & 0x0F, cnt = (rx>>4) & 0x7FF;
            //debugf("rx %08x typ %d cnt %d ep %d\n", rx, typ, cnt, ep);

            switch (typ) {
                case 0b0010:  // OUT
                    debugf("out ep %d cnt %d len %d\n", ep, cnt, setupPkt.len);
                    if (ep == 1)
                        inPending = cnt;
                    else {
                        for (int i = 0; i < cnt; i += 4) {
                            uint32_t x = fifo(0);
                            if (ep == 0 && setupPkt.req == 32 && i == 0)
                                debugf("baudrate %d\n", x);
                            else
                                debugf("drop %d %08x\n", i, x);
                            (void) x; // avoids warning if debugf is empty stub
                        }
                        if (ep == 0) {
                            setupPkt.len -= cnt;
                            if (setupPkt.len == 0)
                                sendEp0(0, 0);
                        }
                    }
                    break;
                case 0b0011:  // OUT complete
                    MMIO32(DOEPCTL0+0x20*ep) |= (1<<26);  // CNAK
                    debugf("out complete %d\n", ep);
                    break;
                case 0b0110:  // SETUP
                    setupPkt.buf[0] = fifo(0);
                    setupPkt.buf[1] = fifo(0);
                    break;
                case 0b0100: {  // SETUP complete
                    MMIO32(DOEPCTL0) |= (1<<26);  // CNAK
                    debugf("setup complete %d t %2x r %d v %04x i %04x l %d\n",
                                ep, setupPkt.typ, setupPkt.req,
                                setupPkt.val, setupPkt.idx, setupPkt.len);
                    const void* replyPtr = 0;
                    uint32_t replyLen = 0;
                    switch (setupPkt.req) {
                        case 5:  // set address
                            MMIO32(DCFG) &= ~(0x7F<<4);  // clear DAD
                            MMIO32(DCFG) |= (setupPkt.val<<4);
                            break;
                        case 6:  // get descriptor
                            switch (setupPkt.val) {
                                case 0x100:  // device desc
                                    replyPtr = devDesc;
                                    replyLen = sizeof devDesc;
                                    break;
                                case 0x200:  // configuration desc
                                    replyPtr = cnfDesc;
                                    replyLen = sizeof cnfDesc;
                                    break;
                            }
                            break;
                        case 9:  // set configuration
                            setConfig();
                            break;
                        case 34:  // set control line state
                            debugf("rts/dtr %02b\n", setupPkt.val);
                            dtr = setupPkt.val & 1;
                            break;
                    }
                    if (setupPkt.len == 0 || (setupPkt.typ & 0x80))
                        sendEp0(replyPtr, replyLen);
                    break;
                }
            }
        }
    }

    static bool writable () {
        poll();
        return (uint16_t) MMIO32(DTXFSTS0+0x20) > 0;
    }

    static void putc (int c) {
        if (dtr) {
            while (!writable()) {}
            MMIO32(DIEPTSIZ0+0x20) = 1;
            fifo(1) = c;
        }
    }

    static bool readable () {
        poll();
        return inReady > 0 || inPending > 0;
    }

    static int getc () {
        while (!readable()) {}

        // get 4 chars from the USB fifo, if needed
        if (inReady == 0) {
            inReady = inPending;
            if (inReady > 4)
                inReady = 4;
            inData = fifo(1);
            inPending -= inReady;
        }

        // consume each of those 4 chars first
        --inReady;
        uint8_t c = inData;
        inData >>= 8;
        return c;
    }
};

uint32_t UsbDev::inPending = 0, UsbDev::inReady = 0, UsbDev::inData;
bool UsbDev::dtr = false;

union UsbDev::Setup UsbDev::setupPkt;

const uint8_t UsbDev::devDesc [] = {
    18, 1, 0, 2, 2, 0, 0, 64,
    0x83, 0x04,  // vendor
    0x40, 0x57,  // product
    0, 2, 0, 0, 0, 1,
};

const uint8_t UsbDev::cnfDesc [] = {
    9, 2, 67, 0, 2, 1, 0, 192, 50, // USB Configuration
    9, 4, 0, 0, 1, 2, 2, 0, 0,     // Interface
    5, 36, 0, 16, 1,               // Header Functional
    5, 36, 1, 0, 1,                // Call Management Functional
    4, 36, 2, 2,                   // ACM Functional
    5, 36, 6, 0, 1,                // Union Functional
    7, 5, 130, 3, 8, 0, 255,       // Endpoint 2
    9, 4, 1, 0, 2, 10, 0, 0, 0,    // Data class interface
    7, 5, 1, 2, 64, 0, 0,          // Endpoint 1 out
    7, 5, 129, 2, 64, 0, 0,        // Endpoint 1 in
};
