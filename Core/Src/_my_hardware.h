#ifndef __MY_HARDWARE_H__
#define __MY_HARDWARE_H__

#include "_my_config.h"     // so we can test if RPM_FILTER is defined

// Note: This will be included by drv_dshot.c, drv_dshot_bidir.c, drv_dshot_dma.c
// *AFTER* some hardcoded definitions have been made (near the top of each
// respective .c file). This make it easy to customize the compile options
// for each of those .c files by adjusting those (and/or other) definitions here
// in this file. Of course that means we no longer need to edit those source files.
//
#undef DSHOT_DMA_BIDIR      // needed for RPM_FILTER, 4k loop frequency max
#undef DSHOT_DMA_DRIVER     // conventional Dshot, consumes less cycles, works for 8k loop frequency
#undef DSHOT_DRIVER         // delay version

#undef BIDIRECTIONAL        // used by: drv_dshot.c, drv_dshot_dma.c, drv_dshot_bidir.c
#undef IDLE_OFFSET          // used by: drv_dshot.c, drv_dshot_dma.c, drv_dshot_bidir.c
#undef DSHOT                // used by:              drv_dshot_dma.c, drv_dshot_bidir.c
#undef MOTOR_POLE_COUNT     // used by:                               drv_dshot_bidir.c

// If RPM_FILTER was defined (see _my_config.h) then we must enable drv_dshot_bidir.c to be compiled
#if defined(RPM_FILTER)
    #define DSHOT_DMA_BIDIR
#else
    #define DSHOT_DMA_DRIVER
#endif

// We're doing our own SPI implementation so we don't need drv_spi.c and
// drv_spi_3wire.c to be compiled. So undef these two to prevent them from compiling
#undef SOFTSPI_3WIRE
#undef SOFTSPI_4WIRE

//------------------------------------------------------------------------------
// Above should usually be left alone but the following can (and should) be
// adjusted to meet your needs
//------------------------------------------------------------------------------
#define IDLE_OFFSET         20  // IDLE_OFFSET is added to the throttle. Adjust its value so that the motors still spin at minimum throttle.
#define DSHOT               300 // Acceptable values are 150, 300 (works on BB1), 600 (BLHeli_S BB2, not supported by BB1), 1200 (BLHeli_32 only)
#define MOTOR_POLE_COUNT    12  // 08XX, 11XX, 12XX motors (typically found on whoop or micros) tend to have 12 poles.


#endif
