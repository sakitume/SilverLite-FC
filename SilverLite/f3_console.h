#include <stdint.h>

struct f3Console
{
    void init();
    void putc(uint8_t ch);
    uint8_t getc();
    bool readable();
};

