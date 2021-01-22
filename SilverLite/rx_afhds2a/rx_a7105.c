/*
 * This file is part of Cleanflight and Betaflight.
 *
 * Cleanflight and Betaflight are free software. You can redistribute
 * this software and/or modify this software under the terms of the
 * GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * Cleanflight and Betaflight are distributed in the hope that they
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software.
 *
 * If not, see <http://www.gnu.org/licenses/>.
 */


/*
 * Based on sources from Betaflight 4.1 maintenance branch, git bc9715e
 * Modified for use in SilverLite FC
 */


#include "_my_config.h"
#if defined(RX_FLYSKY) || defined(RX_FLYSKY2A)

#include <stdbool.h>
#include <stdint.h>
//#include <stdlib.h>

#include "main.h"

#include "drv_time.h"
#define micros  gettime

#include "afhds2a_defs.h"
#include "afhds2a.h"
#include "rx_a7105.h"
#include "rx_spi.h"

#if defined(MATEKF411RX)
    #define RX_SPI_EXTI_IRQn    EXTI15_10_IRQn
#elif defined(CRAZYBEEF3FS)    
    #define RX_SPI_EXTI_IRQn    EXTI9_5_IRQn
#endif    

//------------------------------------------------------------------------------
// Called by console.cpp before it calls jump_to_bootloader()
void A7105Shutdown()
{
    HAL_NVIC_DisableIRQ(RX_SPI_EXTI_IRQn);
    HAL_NVIC_ClearPendingIRQ(RX_SPI_EXTI_IRQn);
}

//------------------------------------------------------------------------------
void EXTIEnable(IO_t ioPin, bool bEnabled)
{
    (void)ioPin;
    if (bEnabled)
    {
        HAL_NVIC_EnableIRQ(RX_SPI_EXTI_IRQn);
    }
    else
    {
        HAL_NVIC_DisableIRQ(RX_SPI_EXTI_IRQn);
    }
}

//------------------------------------------------------------------------------
static IO_t rxIntIO = IO_NONE;
static volatile uint32_t timeEvent = 0;
static volatile bool occurEvent = false;

//------------------------------------------------------------------------------
// HAL_GPIO_EXTI_Callback() is called when an EXTI interrupt occurs. We check
// if the pin that triggered the interrupt is our RX SPI pin and record it
// occurred and at what time.
//
// The RX SPI pin should have been configured wit STM32CubeMX to generate an
// external interrupt on rising trigger
//
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == RX_SPI_EXTI_PIN_Pin) 
    {
        // Betaflight code would perform an IORead() to check the EXTI pin 
        // was high, but the way this interrupt is configured (rising edge)
        // and this callback is told which pin triggered the interrupt, it
        // shouldn't be necessary to read the pin, but I do it anyway
        //if (IORead(rxIntIO) != 0) {
        if (RX_SPI_EXTI_PIN_GPIO_Port->IDR & RX_SPI_EXTI_PIN_Pin) {
            timeEvent = micros();
            occurEvent = true;
        }
    }
}

//------------------------------------------------------------------------------
void A7105Init(uint32_t id, IO_t extiPin, IO_t txEnPin)
{
    (void)extiPin;
    (void)txEnPin;
    
    EXTIEnable(rxIntIO, false);

    A7105SoftReset();
    A7105WriteID(id);
}

void A7105Config(const uint8_t *regsTable, uint8_t size)
{
    if (regsTable) {
        unsigned timeout = 1000;

        for (unsigned i = 0; i < size; i++) {
            if (regsTable[i] != 0xFF) {
                A7105WriteReg ((A7105Reg_t)i, regsTable[i]);
            }
        }

        A7105Strobe(A7105_STANDBY);

        A7105WriteReg(A7105_02_CALC, 0x01);

        while ((A7105ReadReg(A7105_02_CALC) != 0) && timeout--) {}

        A7105ReadReg(A7105_22_IF_CALIB_I);

        A7105WriteReg(A7105_24_VCO_CURCAL, 0x13);
        A7105WriteReg(A7105_25_VCO_SBCAL_I, 0x09);
        A7105Strobe(A7105_STANDBY);
    }
}

bool A7105RxTxFinished(uint32_t *timeStamp) {
    bool result = false;

    if (occurEvent) {
        if (timeStamp) {
            *timeStamp = timeEvent;
        }

        occurEvent = false;
        result = true;
    }
    return result;
}

void A7105SoftReset(void)
{
    rxSpiWriteCommand((uint8_t)A7105_00_MODE, 0x00);
}

uint8_t A7105ReadReg(A7105Reg_t reg)
{
    return rxSpiReadCommand((uint8_t)reg | 0x40, 0xFF);
}

void A7105WriteReg(A7105Reg_t reg, uint8_t data)
{
    rxSpiWriteCommand((uint8_t)reg, data);
}

void A7105Strobe(A7105State_t state)
{
    if (A7105_TX == state || A7105_RX == state) {
        EXTIEnable(rxIntIO, true);
    } else {
        EXTIEnable(rxIntIO, false);
    }

    rxSpiWriteByte((uint8_t)state);
}

void A7105WriteID(uint32_t id)
{
    uint8_t data[4];
    data[0] = (id >> 24) & 0xFF;
    data[1] = (id >> 16) & 0xFF;
    data[2] = (id >> 8) & 0xFF;
    data[3] = (id >> 0) & 0xFF;
    rxSpiWriteCommandMulti((uint8_t)A7105_06_ID_DATA, &data[0], sizeof(data));
}

uint32_t A7105ReadID(void)
{
    uint32_t id;
    uint8_t data[4];
    rxSpiReadCommandMulti((uint8_t)A7105_06_ID_DATA | 0x40, 0xFF, &data[0], sizeof(data));
    id = data[0] << 24 | data[1] << 16 | data[2] << 8 | data[3] << 0;
    return id;
}

void A7105ReadFIFO(uint8_t *data, uint8_t num)
{
    if (data) {
        if(num > 64) {
            num = 64;
        }

        A7105Strobe(A7105_RST_RDPTR); /* reset read pointer */
        rxSpiReadCommandMulti((uint8_t)A7105_05_FIFO_DATA | 0x40, 0xFF, data, num);
    }
}

void A7105WriteFIFO(uint8_t *data, uint8_t num)
{
    if (data) {
        if(num > 64) {
            num = 64;
        }

        A7105Strobe(A7105_RST_WRPTR); /* reset write pointer */
        rxSpiWriteCommandMulti((uint8_t)A7105_05_FIFO_DATA, data, num);
    }
}

#endif /* #if defined(RX_FLYSKY) || defined(RX_FLYSKY2A) */
