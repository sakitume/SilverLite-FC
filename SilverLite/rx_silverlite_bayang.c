#include <math.h> // fabsf
#include <stdbool.h>
#include <stdint.h>
#include "config.h"
#include "rx.h"
#include "trx.h"
#include "util.h"

#ifdef RX_SILVERLITE_BAYANG_PROTOCOL
//------------------------------------------------------------------------------
#define DEBUG_DIAGNOSTICS

int crc_errors;		// incremented by trx_NRF24L01.cpp or trx_LT8900.cpp
int hw_crc_errors;	// packet (hardware or simulated hardware) crc error
int b_crc_errors;	// These are really bayang packet crc errors
int timingfail_errors;
int skipchannel_errors;
int pkt_hits;

#if defined(DEBUG_DIAGNOSTICS)		
static int _b_crc_errors;
static int _pkt_hits;
static int _timingfail_errors;
static int _skipchannel_errors;
#endif

// radio settings

// packet period in uS
#define PACKET_PERIOD 2000
#define PACKET_PERIOD_TELEMETRY 5000

// was 250 ( uS )
#define PACKET_OFFSET 0

#ifdef USE_STOCK_TX
#undef PACKET_PERIOD
#define PACKET_PERIOD 2000
#undef PACKET_OFFSET
#define PACKET_OFFSET 0
#endif

// how many times to hop ahead if no reception
#define HOPPING_NUMBER 4

#define RX_MODE_NORMAL RXMODE_NORMAL
#define RX_MODE_BIND RXMODE_BIND

bool failsafe = true;

float rx[ 4 ];
char aux[ AUXNUMBER ];
float aux_analog[ 2 ] = { 1.0, 1.0 };
#define CH_ANA_AUX1 0
#define CH_ANA_AUX2 1

char lasttrim[ 4 ];

char rfchannel[ 4 ];
uint8_t rxaddress[ 5 ];
int telemetry_enabled = 0;
int rx_bind_load = 0;

int rxmode = 0;
int rf_chan = 0;
int rxdata[19];	// xn297 only uses 15, NRF24L01 emulation needs 17 (or 19 if CRC enabled), LT8900 uses 15

int autobind_inhibit = 0;
int packet_period = PACKET_PERIOD;

static int analog_aux_channels_enabled = 0;
static int silverLiteCapable = 0;
static uint8_t silverLitePIDTuningEnabled = 1;
static uint8_t silverLiteTelemetryCntr;

//------------------------------------------------------------------------------
void rx_init()
{
	// always on (CH_ON) channel set 1
	aux[ AUXNUMBER - 2 ] = 1;
	// always off (CH_OFF) channel set 0
	aux[ AUXNUMBER - 1 ] = 0;
#ifdef AUX1_START_ON
	aux[ CH_AUX1 ] = 1;
#endif
#ifdef AUX2_START_ON
	aux[ CH_AUX2 ] = 1;
#endif

	trx_Init();

	// If rx_bind_load is true then it means startup code retrieved (from flash storage)
	// the rxaddress[] and rfchannel[] of a previously bound TX and we should "autobind"
	// to it.
	if ( rx_bind_load ) {

		trx_SetAddr(rxaddress);
		trx_SetChannel(rfchannel[ rf_chan ]);

		rxmode = RX_MODE_NORMAL;
		if ( telemetry_enabled ) {
			packet_period = PACKET_PERIOD_TELEMETRY;
		}
	} else {
		autobind_inhibit = 1;
	}
}

//#define RXDEBUG

#ifdef RXDEBUG
unsigned long packettime;
int channelcount[ 4 ];
int failcount;
int skipstats[ 12 ];
int afterskip[ 12 ];
#warning "RX debug enabled"
#endif

int packetrx;
int packetpersecond;

static void send_telemetry( void );
static void nextchannel( void );

int loopcounter = 0;
unsigned long send_time;
int telemetry_send = 0;
int send_telemetry_next_loop = 0;
int oldchan = 0;

#define TELEMETRY_TIMEOUT 10000

static void beacon_sequence()
{
	static int beacon_seq_state = 0;

	switch ( beacon_seq_state ) {
		case 0: // send data
			if ( send_telemetry_next_loop || LOOPTIME >= 1000 ) {
				send_telemetry();
				telemetry_send = 1;
				++beacon_seq_state;
			}
			break;
		case 1: // wait for data to finish transmitting
			if ( trx_TelemetrySent() ) {
				beacon_seq_state = 0;
				telemetry_send = 0;
				nextchannel();
				// Be sure to execute nextchannel() before switching to RX mode!
				trx_SetToRXMode();
			}
			else { // if it takes too long we get rid of it
				if ( gettime() - send_time > TELEMETRY_TIMEOUT ) {
					trx_FlushTX();
					trx_SetToRXMode();
					beacon_seq_state = 0;
					telemetry_send = 0;
				}
			}
			break;
		default:
			beacon_seq_state = 0;
			break;
	}
}

extern bool lowbatt;
extern int onground;
extern float vbattfilt;
extern float vbatt_comp;
extern bool telemetry_transmitted; // usermain.c

static void prepare_standard_tlm_packet(int txdata[])
{
	txdata[ 0 ] = 0x85; // 133
	txdata[ 1 ] = lowbatt;

	// battery volt filtered
	int vbatt = vbattfilt * 100 + 0.5f;

#ifndef MOTOR_BEEPS_CHANNEL
#define MOTOR_BEEPS_CHANNEL CH_OFF
#endif

#ifdef LEVELMODE
	if ( aux[ LEVELMODE ] ) {
		extern float accel[ 3 ];
		extern int calibration_done;
		static float maxg = 0.0f;
		if ( fabsf( accel[ 2 ] ) > maxg && calibration_done ) {
			maxg = fabsf( accel[ 2 ] );
		}
		if ( aux[ MOTOR_BEEPS_CHANNEL ] ) { // reset displayed maxg
			maxg = 0.0f;
		}
		vbatt = maxg * 100;
	}
#endif // LEVELMODE

	txdata[ 3 ] = ( vbatt >> 8 ) & 0xff;
	txdata[ 4 ] = vbatt & 0xff;

	// battery volt compensated
	vbatt = vbatt_comp * 100 + 0.5f;
	txdata[ 5 ] = ( vbatt >> 8 ) & 0xff;
	txdata[ 6 ] = vbatt & 0xff;

	int temp = packetpersecond;

#ifdef DISPLAY_MAX_USED_LOOP_TIME_INSTEAD_OF_RX_PACKETS
	extern uint32_t max_used_loop_time;
	if ( aux[ MOTOR_BEEPS_CHANNEL ] ) { // reset displayed max_used_loop_time
		max_used_loop_time = 0;
	}
	temp = max_used_loop_time / 2;
#endif // DISPLAY_MAX_USED_LOOP_TIME_INSTEAD_OF_RX_PACKETS

	if ( temp > 255 ) {
		temp = 255;
	}
	txdata[ 7 ] = temp; // rx strength

	if ( lowbatt && ! onground ) {
		txdata[ 3 ] |= 1 << 3;
	}

#ifdef DISPLAY_PID_VALUES
	extern float * pids_array[ 3 ];
	extern int current_pid_axis, current_pid_term;
	static uint32_t pid_term = 0;
	const bool blink = ( gettime() & 0xFFFFF ) < 200000; // roughly every second (1048575 �s) for 0.2 s
	int pid_value;
	if ( current_pid_term == pid_term && blink ) {
		pid_value = 0;
	} else {
		pid_value = pids_array[ pid_term ][ current_pid_axis ] * 1000 + 0.5f;
	}
	txdata[ 8 ] = ( pid_value >> 8 ) & 0x3F;
	txdata[ 9 ] = pid_value & 0xff;
	txdata[ 8 ] |= pid_term << 6;
	++pid_term;
	if ( pid_term == 3 ) {
		pid_term = 0;
	}
#endif // DISPLAY_PID_VALUES
}

static void prepare_silverLite_tlm_packet(int txdata[])
{
	// Set first byte to indicate this is a SilverLite packet
	// and not a normal telemetry packet (which uses 0x85, which is 133 decimal)
	// We're reserving values 0xA0 thru 0xAF inclusive
	// 0xA0 = normal telemetry
	// 0xA1 = PIDs
	
	// For now we alternate between 0xA0 and 0xA1 even though PIDs
	// never change unless we're manipulating it real-time. Very soon I'll
	// 
	++silverLiteTelemetryCntr;
    txdata[0] = silverLitePIDTuningEnabled && (silverLiteTelemetryCntr & 1) ? 0xA1 : 0xA0;
	
	// 104 bits available for sending (13 bytes from 1 thru 13).
	if (txdata[0] == 0xA1)
	{
		// Arrays are in roll, pitch, yaw order
		// These values should always be in the range of 0.0 to 2.0
		// based on my viewing of sample PID values in pid.c
		// So we can normalize from that range to 0 to 2043 and
		// use only 11 bits to save space
		extern float pidkp[];
		extern float pidki[];
		extern float pidkd[];
		
		// STM32 is (of course) a 32 bit processor so we'll just use
		// 'int' rather than an explicit type (such as uint16_t) so that
		// which shifts bits over (8-bit) byte boundaries just works
		// without us having to perform additional operations which could
		// potentially confuse the C compiler from generating more optimal code
		// I could review the generated code or even inline some assembly but
		// I doubt any difference in gains/losses is worth it
		//
		// Note: I'm not treating our packet buffer as a bit stream. Instead I'm
		// packing the lower 8 bits of each value first and accumulating the upper
		// 3 bits of each value (27 bits in total) for storing afterwards

		int rP;
		int pP;
		if (analog_aux_channels_enabled)
		{
			rP = (int)(pidkp[0] * aux_analog[0] * 1000 + 0.5f);
			pP = (int)(pidkp[1] * aux_analog[0] * 1000 + 0.5f);
		}
		else
		{
			rP = (int)(pidkp[0] * 1000 + 0.5f);
			pP = (int)(pidkp[1] * 1000 + 0.5f);
		}

		const int yP = (int)(pidkp[2] * 1000 + 0.5f);
		txdata[1] = rP & 0xFF;	// Masking with 0xFF is necessary because txdata[] is not a uint8_t array
		txdata[2] = pP & 0XFF;
		txdata[3] = yP & 0XFF;
		unsigned bits = 
			((rP & 0x700) >> 8) |
			((pP & 0x700) >> 5) |
			((yP & 0x700) >> 2);
		
		const int rI = (uint16_t)(pidki[0] * 1000 + 0.5f);
		const int pI = (uint16_t)(pidki[1] * 1000 + 0.5f);
		const int yI = (uint16_t)(pidki[2] * 1000 + 0.5f);
		txdata[4] = rI & 0xFF;
		txdata[5] = pI & 0XFF;
		txdata[6] = yI & 0XFF;
		bits |= 
			((rI & 0x700) << 1) |
			((pI & 0x700) << 4) |
			((yI & 0x700) << 7);

		int rD;
		int pD;
		if (analog_aux_channels_enabled)
		{
			rD = (uint16_t)(pidkd[0] * aux_analog[1] * 1000 + 0.5f);
			pD = (uint16_t)(pidkd[1] * aux_analog[1] * 1000 + 0.5f);
		}
		else
		{
			rD = (uint16_t)(pidkd[0] * 1000 + 0.5f);
			pD = (uint16_t)(pidkd[1] * 1000 + 0.5f);
		}
		const int yD = (uint16_t)(pidkd[2] * 1000 + 0.5f);
		txdata[7] = rD & 0xFF;
		txdata[8] = pD & 0XFF;
		txdata[9] = yD & 0XFF;
		bits |= 
			((rD & 0x700) << 10) |
			((pD & 0x700) << 13) |
			((yD & 0x700) << 16);
			
		txdata[10] = (bits >> 0) & 0xFF;
		txdata[11] = (bits >> 8) & 0xFF;
		txdata[12] = (bits >> 16) & 0xFF;
		txdata[13] = (bits >> 24) & 0x7;	// upper 5 bits should already be clear so mask is irrelevant
	}
	else
	{
		uint16_t pktsPerSec = packetpersecond < 1024 ? packetpersecond : 1023;
		
		// battery voltage filt and battery voltage comp
		// Again, bits of each value are not contiguous
		uint16_t vbattFilt = (uint16_t)(vbattfilt * 100);
		uint16_t vbattComp = (uint16_t)(vbatt_comp * 100);
		txdata[1] = (vbattFilt & 0xFF);
		txdata[2] = (vbattComp & 0xFF);
		txdata[3] = (pktsPerSec & 0xFF);
		txdata[4] = 
			((vbattFilt & 0x700) >> 8) |
			((vbattComp & 0x700) >> 5) |
			((pktsPerSec& 0x300) >> 2);
		
		// Flags
		extern int onground;
		txdata[5] =
			(lowbatt ? 			(1 << 5) : 0) |
			(onground ? 		(1 << 4) : 0) |
#if 0	// this was for NFE fork			
			(aux[LEVELMODE] ? 	(1 << 3) : 0) |
			(aux[RACEMODE] ? 	(1 << 2) : 0) |
			(aux[HORIZON] ? 	(1 << 1) : 0) |
			(aux[RATES] ? 		(1 << 0) : 0);
#else
			0;			
#endif			
			
		for (int i=6; i<14; i++)
		{
			txdata[i] = 0;
		}
	}
}

static void send_telemetry()
{
	trx_PrepareToSendTelemetry();

	int txdata[ 15 ];
	for ( int i = 0; i < 15; ++i ) {
		txdata[ i ] = i;
	}

	if (silverLiteCapable)
	{
		prepare_silverLite_tlm_packet(txdata);
	}
	else
	{
		prepare_standard_tlm_packet(txdata);
	}

	int sum = 0;
	for ( int i = 0; i < 14; ++i ) {
		sum += txdata[ i ];
	}

	txdata[ 14 ] = sum;

	trx_SendTelemetryPacket(txdata, 15);

	send_time = gettime();

	telemetry_transmitted = true;
}

static char checkpacket()
{
	return trx_PacketAvailable();
}

static float packettodata( int * data )
{
	return ( ( ( data[ 0 ] & 0x0003 ) * 256 + data[ 1 ] ) / 1023.0f * 2.0f ) - 1.0f;
}

static float bytetodata( int byte )
{
	// 0.0 .. 2.0
	return byte / 255.0f * 2.0f;
}

static int decodepacket( void )
{
	if ( rxdata[ 0 ] == 165 ) {
		int sum = 0;
		for ( int i = 0; i < 14; ++i ) {
			sum += rxdata[ i ];
		}
		if ( ( sum & 0xFF ) == rxdata[ 14 ] ) {
			// roll, pitch, yaw: -1 .. 1
			rx[ 0 ] = packettodata( &rxdata[ 4 ] );
			rx[ 1 ] = packettodata( &rxdata[ 6 ] );
			rx[ 2 ] = packettodata( &rxdata[ 10 ] );
			// throttle: 0 .. 1
			rx[ 3 ] = ( ( rxdata[ 8 ] & 0x0003 ) * 256 + rxdata[ 9 ] ) / 1023.0f;

#ifdef USE_STOCK_TX
			char trims[ 4 ];
			trims[ 0 ] = rxdata[ 6 ] >> 2;
			trims[ 1 ] = rxdata[ 4 ] >> 2;

			for ( int i = 0; i < 2; ++i ) {
				if ( trims[ i ] != lasttrim[ i ] ) {
					aux[ CH_PIT_TRIM + i ] = trims[ i ] > lasttrim[ i ];
					lasttrim[ i ] = trims[ i ];
				}
			}
#else
	#ifdef INVERT_CH_INV
			aux[ CH_INV ] = ( rxdata[ 3 ] & 0x80 ) ? 0 : 1; // inverted flag
	#else
			aux[ CH_INV ] = ( rxdata[ 3 ] & 0x80 ) ? 1 : 0; // inverted flag
	#endif			
			aux[ CH_VID ] = ( rxdata[ 2 ] & 0x10 ) ? 1 : 0;
			aux[ CH_PIC ] = ( rxdata[ 2 ] & 0x20 ) ? 1 : 0;
#endif

			aux[ CH_TO ] = ( rxdata[ 3 ] & 0x20 ) ? 1 : 0; // take off flag
			aux[ CH_EMG ] = ( rxdata[ 3 ] & 0x04 ) ? 1 : 0; // emg stop flag
			
			aux[ CH_FLIP ] = ( rxdata[ 2 ] & 0x08 ) ? 1 : 0;
			aux[ CH_EXPERT ] = ( rxdata[ 1 ] == 0xfa ) ? 1 : 0;

#ifdef INVERT_CH_HEADFREE
			aux[ CH_HEADFREE ] = ( rxdata[ 2 ] & 0x02 ) ? 0 : 1;
#else
			aux[ CH_HEADFREE ] = ( rxdata[ 2 ] & 0x02 ) ? 1 : 0;
#endif

			aux[ CH_RTH ] = ( rxdata[ 2 ] & 0x01 ) ? 1 : 0; // rth channel


			if (analog_aux_channels_enabled)
			{
				aux_analog[ CH_ANA_AUX1 ] = bytetodata( rxdata[ 1 ] );
				aux_analog[ CH_ANA_AUX2 ] = bytetodata( rxdata[ 13 ] );
			}

			// Check for silverLite flag. Although we do something similar when we check bind packets
			// we also check here in case auto-bind kicked in and bind packets were skipped altogether
			if (rxdata[12] == (rxaddress[2] ^ 0xAA))
			{
				if (!silverLiteCapable)
				{
					XPRINTF("AutoBound to SilverLite capable TX\n");
					silverLiteCapable = 1;
				}
			}

#ifdef LEVELMODE
			// level mode expo
			if (aux[ LEVELMODE ])
			{ 
				if (ANGLE_EXPO_ROLL > 0.01) 	rx[0] = rcexpo(rx[0], ANGLE_EXPO_ROLL);
				if (ANGLE_EXPO_PITCH > 0.01) 	rx[1] = rcexpo(rx[1], ANGLE_EXPO_PITCH);
				if (ANGLE_EXPO_YAW > 0.01) 		rx[2] = rcexpo(rx[2], ANGLE_EXPO_YAW);
			}
			else
#endif			
			{
				if (ACRO_EXPO_ROLL > 0.01) 	rx[0] = rcexpo(rx[0], ACRO_EXPO_ROLL);
				if (ACRO_EXPO_PITCH > 0.01)	rx[1] = rcexpo(rx[1], ACRO_EXPO_PITCH);
				if (ACRO_EXPO_YAW > 0.01) 	rx[2] = rcexpo(rx[2], ACRO_EXPO_YAW);
			}


			return 1; // valid packet
		}
#if defined(DEBUG_DIAGNOSTICS)		
		if (_b_crc_errors++ == 0)
		{
			XPRINTF("CRC: ");
			for (int i=0; i<15; i++)
			{
				XPRINTF("%x ",rxdata[i]);
			}
			XPRINTF("\n");
		}
#endif		
		return 0; // sum fail
	}
#if defined(DEBUG_DIAGNOSTICS)		
	if (_b_crc_errors++ == 0)
	{
		XPRINTF("HDR: ");
		for (int i=0; i<15; i++)
		{
			XPRINTF("%x ",rxdata[i]);
		}
		XPRINTF("\n");
	}
#endif	
	return 0; // first byte different
}

static void nextchannel()
{
	++rf_chan;
	rf_chan &= 3; // same as %4
	trx_SetChannel(rfchannel[ rf_chan ] );
}

unsigned long lastrxtime;
unsigned long failsafetime;
unsigned long secondtimer;

unsigned int skipchannel = 0;
int lastrxchan;
int timingfail = 0;

void checkrx( void )
{
	if ( LOOPTIME < 500 ) { // Kludge. 500 works perfectly for telemetry on nrf24, so we skip some calls accordingly.
		static int count;
		if ( count == 0 ) {
			count = 500 / LOOPTIME - 1;
		} else {
			--count;
			return;
		}
	}

	if ( send_telemetry_next_loop ) {
		beacon_sequence();
		send_telemetry_next_loop = 0;
		return;
	}

	// Don't check for a packet if we sent telemery and are waiting
	// for the telemetry packet to finish being sent
	const int packetreceived = telemetry_send ? 0 : checkpacket();

#ifdef RX_PREDICTOR
	static unsigned long last_good_rx_time;
	static unsigned long next_predictor_time;
	static float last_good_rx[ 4 ];
	static float rx_velocity[ 4 ];
#endif	

	if ( packetreceived ) {
#if defined(DEBUG_DIAGNOSTICS)		
		_pkt_hits++;
#endif		
		if ( rxmode == RX_MODE_BIND ) { // rx startup, bind mode
			if (!trx_ReadPayload(rxdata, 15)) {
				return;
			}

#if 1		
			// CRC check this bind packet	
			{ 
				int sum = 0;
				for ( int i = 0; i < 14; ++i ) {
					sum += rxdata[ i ];
				}
				if ( ( sum & 0xFF ) != rxdata[ 14 ] ) {
					XPRINTF("Bind packet CRC failure");
					return;
				}
			}
#endif			


			if ( rxdata[ 0 ] == 0xa4 || rxdata[ 0 ] == 0xa3 || rxdata[ 0 ] == 0xa2 || rxdata[ 0 ] == 0xa1 ) { // bind packet
				if ( rxdata[ 0 ] == 0xa3 || rxdata[ 0 ] == 0xa1 ) {
					telemetry_enabled = 1;
					packet_period = PACKET_PERIOD_TELEMETRY;
				}
				
				if ( rxdata[ 0 ] == 0xa1 || rxdata[ 0 ] == 0xa2 ) {
					analog_aux_channels_enabled = 1;
				}
				else
				{
					aux_analog[ CH_ANA_AUX1 ] = 1.0f;
					aux_analog[ CH_ANA_AUX2 ] = 1.0f;
				}

				rfchannel[ 0 ] = rxdata[ 6 ];
				rfchannel[ 1 ] = rxdata[ 7 ];
				rfchannel[ 2 ] = rxdata[ 8 ];
				rfchannel[ 3 ] = rxdata[ 9 ];

				for ( int i = 0; i < 5; ++i ) {
					rxaddress[ i ] = rxdata[ i + 1 ];
				}

				// Set address and channel
				trx_SetAddr(rxaddress);
				trx_SetChannel(rfchannel[ rf_chan ]);

				// Examine rxdata[10] and rxdata[11] to see if they have the
				// special magic bytes that indicate TX is SilverLite capable
				if (((rxdata[1] ^ 0xAA) == rxdata[10]) && ((rxdata[2] ^ 0xAA) == rxdata[11]))
				{
					XPRINTF("Bound to SilverLite capable TX\n");
					silverLiteCapable = 1;
				}


#if 1				
				XPRINTF("Bound to TX ID:");
				for (int i=0; i<4; i++)
				{
					XPRINTF("0x%0x ", rxaddress[i]);
				}
				XPRINTF("\n");
#endif				

				rxmode = RX_MODE_NORMAL;
			}
		} else { // normal mode
#ifdef RXDEBUG
			++channelcount[ rf_chan ];
			packettime = gettime() - lastrxtime;
			if ( skipchannel && ! timingfail ) {
				++afterskip[ skipchannel ];
			}
			if ( timingfail ) {
				++afterskip[ 0 ];
			}
#endif

			unsigned long temptime = gettime();

			int pass = trx_ReadPayload(rxdata, 15);
			if ( pass ) {
				pass = decodepacket();
			}

			if ( pass ) {
				++packetrx;
				if ( telemetry_enabled ) {
					if ( LOOPTIME >= 1000 ) {
						beacon_sequence();
					} else {
						send_telemetry_next_loop = 1;
					}
				}
				skipchannel = 0;
				timingfail = 0;
				lastrxchan = rf_chan;
				lastrxtime = temptime;
				failsafetime = temptime;
				failsafe = false;

				// If not performing telemetry then hop to next channel
				if ( ! telemetry_send && ! send_telemetry_next_loop ) {
				 	nextchannel();
				}

#ifdef RX_PREDICTOR
				const float timefactor = 1.0f / ( temptime - last_good_rx_time );
				last_good_rx_time = temptime;
				next_predictor_time = last_good_rx_time + packet_period;
				for ( int i = 0; i < 4; ++i ) {
					rx_velocity[ i ] = ( rx[ i ] - last_good_rx[ i ] ) * timefactor;
					last_good_rx[ i ] = rx[ i ];
				}
#endif // RX_PREDICTOR
			} else {
#ifdef RXDEBUG
				++failcount;
#endif
			}
		} // end normal rx mode
	} // end packet received

	// finish sending if already started
	if ( telemetry_send ) {
		beacon_sequence();
	}

	unsigned long time = gettime();

	if ( (time - lastrxtime) > (unsigned)(HOPPING_NUMBER * packet_period + 1000) && rxmode != RX_MODE_BIND ) {
		// channel with no reception
		lastrxtime = time;
		// set channel to last with reception
		if ( ! timingfail ) {
			rf_chan = lastrxchan;
		}
		// advance to next channel
		nextchannel();
		// set flag to discard packet timing
		timingfail = 1;
#if defined(DEBUG_DIAGNOSTICS)		
		_timingfail_errors++;
#endif		
	}

	if ( ! timingfail && ! telemetry_send && skipchannel < HOPPING_NUMBER + 1 && rxmode != RX_MODE_BIND ) {
		unsigned int temp = time - lastrxtime;

		if ( temp > 1000 && ( temp - ( PACKET_OFFSET ) ) / ( (int)packet_period ) >= ( skipchannel + 1 ) ) {
			nextchannel();
#ifdef RXDEBUG
			++skipstats[ skipchannel ];
#endif
			++skipchannel;
#if defined(DEBUG_DIAGNOSTICS)		
			_skipchannel_errors++;
#endif			
		}
	}

// This block was introduced in SilF4Ware but my early tests show it leads to lower overall PPS
// if the looptime is too close to the packet period. I suspect an aliasing effect occurs where
// we just missed a packet during this iteration which we would pick up on the next iteration
// Changing the looptime from 1000 to 650 (or even 800 or 1100) increases PPS (packets per second)
// so until I can review this further I would like to keep this code intact
#define __FLUSHRX_IF_MISSED_PACKET__
#if defined(__FLUSHRX_IF_MISSED_PACKET__)
	// The RX FIFO needs to be flushed, otherwise we may read an outdated packet when regaining the signal.
	static bool rx_already_flushed = false;
	if ( (time - lastrxtime) > (unsigned)packet_period ) {
		if ( ! rx_already_flushed ) {
			trx_FlushRX();
			rx_already_flushed = true;
		}
	} else {
		rx_already_flushed = false;
	}
#endif	

#ifdef RX_PREDICTOR
	if ( time >= next_predictor_time &&
		next_predictor_time - last_good_rx_time <= 15 * packet_period ) // Stop predicting after noo many missed packets in a row.
	{
		next_predictor_time += packet_period; // Predict new values only at packet_period intervals.
		for ( int i = 0; i < 4; ++i ) {
			// Check for sane stick velocity
			if ( fabsf( rx_velocity[ i ] ) < 50.0f / 500.0f / 5000.0f ) { // v < 20.0/sec i.e. 0.05 sec from 0.0 to 1.0
				rx[ i ] = last_good_rx[ i ] + rx_velocity[ i ] * ( time - last_good_rx_time );
				if ( ( last_good_rx[ i ] > 0.0f && rx[ i ] < 0.0f ) || ( last_good_rx[ i ] < 0.0f && rx[ i ] > 0.0f ) ) {
					rx[ i ] = 0.0f;
				} else if ( rx[ i ] > 1.0f ) {
					rx[ i ] = 1.0f;
				} else if ( rx[ i ] < -1.0f ) {
					rx[ i ] = -1.0f;
				}
			}
		}
	}
#endif // RX_PREDICTOR

	if ( time - failsafetime > FAILSAFETIME ) { // failsafe
		failsafe = true;
		for ( int i = 0; i < 4; ++i ) {
			rx[ i ] = 0.0f;
#ifdef RX_PREDICTOR
			last_good_rx[ i ] = 0.0f;
			rx_velocity[ i ] = 0.0f;
#endif			
		}
	}

	if ( ! failsafe ) {
		autobind_inhibit = 1;
	} else if ( ! autobind_inhibit && time > 15000000 ) {
		autobind_inhibit = 1;
		rxmode = RX_MODE_BIND;

		trx_SetAddr(0);			// 0 means set to the bind address
		trx_SetChannel(0);		// bind on channel 0
		trx_SetToRXMode();		// shouldn't be necessary
	}

	if ( gettime() - secondtimer > 1000000 ) {
		hw_crc_errors = crc_errors;
		crc_errors = 0;
		packetpersecond = packetrx;
#if defined(DEBUG_DIAGNOSTICS)		
		timingfail_errors = _timingfail_errors;
		_timingfail_errors = 0;
		skipchannel_errors = _skipchannel_errors;
		_skipchannel_errors = 0;
		b_crc_errors = _b_crc_errors;
		_b_crc_errors = 0;
		pkt_hits = _pkt_hits;
		_pkt_hits = 0;
#endif		
		packetrx = 0;
		secondtimer = gettime();
	}
}

#endif // #ifdef RX_SILVERLITE_BAYANG

