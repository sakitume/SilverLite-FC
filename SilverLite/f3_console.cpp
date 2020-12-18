#include "f3_console.h"

extern "C" {
extern void f3Console_putc(uint8_t ch);
extern int f3Console_avail();
extern uint8_t f3Console_get();
int xprintf(const char* fmt, ...);
}

//------------------------------------------------------------------------------
void f3Console::init()
{
}

//------------------------------------------------------------------------------
void f3Console::putc(uint8_t ch)
{
    f3Console_putc(ch);
}

//------------------------------------------------------------------------------
uint8_t f3Console::getc()
{
    return f3Console_get();
}

//------------------------------------------------------------------------------
bool  f3Console::readable()
{
    return f3Console_avail();
}
