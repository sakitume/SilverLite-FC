#include <stdbool.h>

#include "config.h"
#include "drv_adc.h"
#include "drv_time.h"
#include "filter.h"

float bb_battadc;
float vbattfilt = 4.2; // filtered battery voltage
float vbatt_comp = 4.2; // compensated for sag by motor sum
static float vbattfilt_corr = 4.2; // Huge time constant filtered, used in Li-Ion model

bool lowbatt = false;
float battery_scale_factor = 1.0;

#define CELL_COUNT_UNSCALED 4 // Voltage divider, idle_offset, and PID values tuned for 4S.

static float vbatt_read( void )
{
	float voltage =
#ifdef BATTERY_CELL_COUNT_DETECTION
	adc_read() * (float)ADC_SCALEFACTOR / (float)CELL_COUNT_UNSCALED * battery_scale_factor;
#else
	adc_read() * (float)ADC_SCALEFACTOR * battery_scale_factor;
#endif

#ifdef USE_TWO_POINT_VOLTAGE_CORRECTION
	if (voltage < REPORTED_TELEMETRY_VOLTAGE_LO)
	{
		return voltage;
	}
	#define ACTUAL_BATTERY_VOLTAGE_RANGE 		(ACTUAL_BATTERY_VOLTAGE_HI - ACTUAL_BATTERY_VOLTAGE_LO)
	#define REPORTED_TELEMETRY_VOLTAGE_RANGE 	(REPORTED_TELEMETRY_VOLTAGE_HI - REPORTED_TELEMETRY_VOLTAGE_LO)
	voltage = (voltage - REPORTED_TELEMETRY_VOLTAGE_LO) * (ACTUAL_BATTERY_VOLTAGE_RANGE / REPORTED_TELEMETRY_VOLTAGE_RANGE)	+ ACTUAL_BATTERY_VOLTAGE_LO;
#endif
	return voltage;
}

void battery_init( void )
{
	int count = 0;
	vbattfilt = 0.0f;

	while ( count < 64 ) {
		vbattfilt += vbatt_read();
		delay( 1000 );
		++count;
	}
	vbattfilt = vbattfilt / count;

#ifdef BATTERY_CELL_COUNT_DETECTION
	//for cells in range( 1, 5 ):
	//	print "'%sS'" % cells
	//	for cell_voltage in ( 3.2, 4.25 ):
	//		voltage = cell_voltage * cells
	//		print 'I' * int( voltage * 10 ), voltage, voltage, '(%s * %s)' % ( cell_voltage, cells )
	//	print
	for ( int cell_count = 4; cell_count > 0; --cell_count ) { // 3.2V works for up to 4S without overlapping ranges
		if ( cell_count * 3.2f < vbattfilt * CELL_COUNT_UNSCALED || cell_count == 1 ) {
			battery_scale_factor = (float)CELL_COUNT_UNSCALED / cell_count;
			break;
		}
	}
	vbattfilt *= battery_scale_factor;
	extern int idle_offset; // drv_dshot.c
	idle_offset *= battery_scale_factor;
#endif // BATTERY_CELL_COUNT_DETECTION

	vbatt_comp = vbattfilt_corr = vbattfilt;

#ifdef STOP_LOWBATTERY
	extern void failloop( int );
	if ( vbattfilt < 3.3f ) {
		failloop( 2 ); // infinite loop
	}
#endif
}

void battery( void )
{
	const float battadc = vbatt_read();
	bb_battadc = battadc;
	lpf( &vbattfilt, battadc, ALPHACALC( LOOPTIME, 2 * PI_F * 0.5e6f ) ); // 0.5 seconds time constant (tau)

	// battery low logic

	// Average of all 4 motor thrusts. Should be proportional with battery sagging.
	extern float thrsum; // control.c
	static float thrfilt = 0; // filtered average of all motors

	// filter motorpwm so it has the same delay as the filtered voltage
	lpf( &thrfilt, thrsum, ALPHACALC( LOOPTIME, 2 * PI_F * 0.5e6f ) );

	// li-ion battery model compensation time decay (18 seconds)
	lpf( &vbattfilt_corr, vbattfilt, ALPHACALC( LOOPTIME, 18e6f ) );

	// compensation factor for li-ion internal model
	// zero to bypass
	#define CF1 0.25f

	const float tempvolt = vbattfilt * ( 1.0f + CF1 ) - vbattfilt_corr * CF1;

#ifdef AUTO_VDROP_FACTOR

	#define BINS 12
	static float lastout[ BINS ];
	static float lastin[ BINS ];
	static float vcomp[ BINS ];
	static float score[ BINS ];
	static int z = 0;
	static int minindex = 0;
	static int firstrun = 1;

	if ( thrfilt > 0.1f ) {
		vcomp[ z ] = tempvolt + (float)z * 0.1f * thrfilt;

		if ( firstrun ) {
			for ( int y = 0; y < BINS; ++y ) {
				lastin[ y ] = vcomp[ z ];
			}
			firstrun = 0;
		}
		// y(n) = x(n) - x(n-1) + R * y(n-1)
		// out = in - lastin + coeff * lastout
		// hpf
		const float ans = vcomp[ z ] - lastin[ z ] + ( 1 - ALPHACALC( LOOPTIME * BINS, 6e6f ) ) * lastout[ z ];
		lastin[ z ] = vcomp[ z ];
		lastout[ z ] = ans;
		lpf( &score[ z ], ans * ans, ALPHACALC( LOOPTIME * BINS, 60e6f ) );
		++z;

		if ( z >= BINS ) {
			z = 0;
			float min = score[ 0 ];
			for ( int i = 0; i < BINS; ++i ) {
				if ( score[ i ] < min ) {
					min = score[ i ];
					minindex = i;
					// add an offset because it seems to be usually early
					++minindex;
				}
			}
		}
	}

	#undef VDROP_FACTOR
	#define VDROP_FACTOR minindex * 0.1f

#endif // AUTO_VDROP_FACTOR

	vbatt_comp = tempvolt + (float)VDROP_FACTOR * thrfilt;

#ifdef WARN_ON_LOW_BATTERY
	const float hyst = lowbatt ? (float)VOLTAGE_HYSTERESIS : 0.0f;
	if ( vbatt_comp < (float)WARN_ON_LOW_BATTERY + hyst || vbattfilt < 2.7f ) {
		lowbatt = 1;
	} else {
		lowbatt = 0;
	}
#endif // WARN_ON_LOW_BATTERY
}
