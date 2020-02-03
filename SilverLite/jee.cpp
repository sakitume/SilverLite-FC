#include "jee.h"
#include <stdarg.h>
#include <string.h>

// interrupt vector table in ram

VTable& VTableRam () {
    static VTable vtable __attribute__((aligned (512)));

    // if SCB_VTOR isn't pointing to vtable, then copy current vtable to it
    VTable* volatile& vtor = *(VTable* volatile*) 0xE000ED08;
    if (vtor != &vtable) {
        vtable = *vtor;
        vtor = &vtable;
    }

    return vtable;
}

// systick and delays

uint32_t volatile ticks;

void enableSysTick (uint32_t divider) {
    VTableRam().systick = []() { ++ticks; };
    constexpr static uint32_t tick = 0xE000E010;
    MMIO32(tick+0x04) = MMIO32(tick+0x08) = divider - 1;
    MMIO32(tick+0x00) = 7;
}

