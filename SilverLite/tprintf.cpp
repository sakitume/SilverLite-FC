#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>

static int splitInt (uint32_t val, int base, uint8_t* buf) {
    int i = 0;
    do {
        buf[i++] = val % base;
        val /= base;
    } while (val != 0);
    return i;
}

static void putFiller (void (*emit)(int), int n, char fill) {
    while (--n >= 0)
        emit(fill);
}

static void putInt (void (*emit)(int), int val, int base, int width, char fill) {
    uint8_t buf [33];
    int n;
    if (val < 0 && base == 10) {
        n = splitInt(-val, base, buf);
        if (fill != ' ')
            emit('-');
        putFiller(emit, width - n - 1, fill);
        if (fill == ' ')
            emit('-');
    } else {
        n = splitInt(val, base, buf);
        putFiller(emit, width - n, fill);
    }
    while (n > 0) {
        uint8_t b = buf[--n];
        emit("0123456789ABCDEF"[b]);
    }
}

void veprintf(void (*emit)(int), char const* fmt, va_list ap) {
    char const* s;

    while (*fmt) {
        char c = *fmt++;
        if (c == '%') {
            char fill = *fmt == '0' ? '0' : ' ';
            int width = 0, base = 0;
            while (base == 0) {
                c = *fmt++;
                switch (c) {
                    case 'b':
                        base =  2;
                        break;
                    case 'o':
                        base =  8;
                        break;
                    case 'd':
                        base = 10;
                        break;
                    case 'x':
                        base = 16;
                        break;
                    case 'c':
                        putFiller(emit, width - 1, fill);
                        c = va_arg(ap, int);
                        // fall through
                    case '%':
                        emit(c);
                        base = 1;
                        break;
                    case 's':
                        s = va_arg(ap, char const*);
                        width -= strlen(s);
                        while (*s)
                            emit(*s++);
                        putFiller(emit, width, fill);
                        // fall through
                    default:
                        if ('0' <= c && c <= '9')
                            width = 10 * width + c - '0';
                        else
                            base = 1; // stop scanning
                }
            }
            if (base > 1) {
                int val = va_arg(ap, int);
                putInt(emit, val, base, width, fill);
            }
        } else
            emit(c);
    }
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
const int kTempBuffMax = 80;
static char tempBuff[80+1];
static int tempBuffIndex;
static void emitToTempBuff(int ch)
{
	if (tempBuffIndex < kTempBuffMax)
	{
		tempBuff[tempBuffIndex++] = ch;
	}
}

extern "C" const char *tprintf(const char* fmt, ...) {
    tempBuffIndex = 0;
    va_list ap; va_start(ap, fmt); 
    veprintf(emitToTempBuff, fmt, ap); 
    va_end(ap);
    tempBuff[tempBuffIndex++] = 0;
    return tempBuff;
}

#if defined(ARDUINO)
#include <Arduino.h>
//------------------------------------------------------------------------------
static void emitToSerial(int ch)
{
    Serial.print((char)ch);
}

extern "C" int xprintf(const char* fmt, ...) {
    va_list ap; va_start(ap, fmt); 
    veprintf(emitToSerial, fmt, ap); 
    va_end(ap);
    return 0;
}
#endif
