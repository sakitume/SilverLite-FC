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

#include <stdbool.h>
#include <stdint.h>

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    RX_SPI_A7105_FLYSKY,
    RX_SPI_A7105_FLYSKY_2A
} rx_spi_protocol_e;

typedef enum {
    RX_SPI_RECEIVED_NONE = 0,
    RX_SPI_RECEIVED_BIND = (1 << 0),
    RX_SPI_RECEIVED_DATA = (1 << 1),
    RX_SPI_ROCESSING_REQUIRED = (1 << 2),
} rx_spi_received_e;

// io_types.h
typedef uint8_t ioTag_t;        // packet tag to specify IO pin
typedef void* IO_t;             // type specifying IO pin. Currently ioRec_t pointer, but this may change
#define IO_NONE ((IO_t)0)       // NONE initializer for IO_t variable

// rx_spi.h
typedef struct rxSpiConfig_s {
    // RX protocol
    uint8_t rx_spi_protocol;                // type of SPI RX protocol
                                            // nrf24: 0 = v202 250kbps. (Must be enabled by FEATURE_RX_NRF24 first.)
//    uint32_t rx_spi_id;
//    uint8_t rx_spi_rf_channel_count;

    // SPI Bus
//    ioTag_t csnTag;
//    uint8_t spibus;

//    ioTag_t bindIoTag;
//    ioTag_t ledIoTag;
//    uint8_t ledInversion;

    ioTag_t extiIoTag;
} rxSpiConfig_t;

// rx.h
typedef struct rxRuntimeConfig_s {
//    rxProvider_t        rxProvider;
//    SerialRXType        serialrxProvider;
    uint8_t             channelCount; // number of RC channels as reported by current input driver
//    uint16_t            rxRefreshRate;
//    rcReadRawDataFnPtr  rcReadRawFn;
//    rcFrameStatusFnPtr  rcFrameStatusFn;
//    rcProcessFrameFnPtr rcProcessFrameFn;
//    uint16_t            *channelData;
//    void                *frameData;
} rxRuntimeConfig_t;

typedef struct flySkyConfig_s {
    uint32_t txId;
    uint8_t rfChannelMap[16];
} flySkyConfig_t;
const flySkyConfig_t* flySkyConfig(void);
flySkyConfig_t* flySkyConfigMutable(void);

bool flySkyInit(const struct rxSpiConfig_s *rxConfig, struct rxRuntimeConfig_s *rxRuntimeConfig);
void flySkySetRcDataFromPayload(uint16_t *rcData, const uint8_t *payload);
rx_spi_received_e flySkyDataReceived(uint8_t *payload);

#ifdef __cplusplus
}
#endif
