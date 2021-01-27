#include <math.h> // fabsf, sqrtf
#include <stdbool.h>
#include <stdint.h>

#include "angle_pid.h"
#include "config.h"
#include "drv_dshot.h"
#include "drv_time.h"
#include "filter.h"
#include "pid.h"
#include "stick_vector.h"
#include "util.h"

#include "debug.h"

int onground = 1;
int pwmdir = FORWARD;
bool reverse_motor_direction[ 4 ] = {
	[ MOTOR_BL - 1 ] = REVERSE_MOTOR_BL,
	[ MOTOR_FL - 1 ] = REVERSE_MOTOR_FL,
	[ MOTOR_BR - 1 ] = REVERSE_MOTOR_BR,
	[ MOTOR_FR - 1 ] = REVERSE_MOTOR_FR
};

float thrsum;
static float mixmax;
static float throttle_reversing_kick;
static float throttle_reversing_kick_sawtooth;
static float throttle_reversing_kick_decrement;
static int prevent_motor_filtering_state = 0;

float error[ PIDNUMBER ];
float setpoint[ PIDNUMBER ];

extern float vbattfilt; // battery.c
extern float vbatt_comp;
extern float vbattfilt_corr;
extern float battery_scale_factor;

extern float gyro[ 3 ]; // sixaxis.c

extern float pidoutput[ PIDNUMBER ]; // pid.c
extern float ierror[ PIDNUMBER ]; // pid.c

extern bool failsafe; // rx.c
extern float rx[ 4 ]; // rx.c
extern char aux[ AUXNUMBER ]; // rx.c
extern float aux_analog[ 2 ];
extern int packet_period; // rx.c

extern bool ledcommand; // led.c

float idle_offset = IDLE_OFFSET; // gets corrected by battery_scale_factor in battery.c
float rxcopy[ 4 ];
float bb_throttle;
float bb_mix[ 4 ];

static void throttle_kick( float kick_strength )
{
	throttle_reversing_kick = kick_strength * ( ( battery_scale_factor - 1.0f ) * 1.5f + 1.0f );
	#define TRKD 100000.0f // 100 ms throttle reversing kick duration
	throttle_reversing_kick_sawtooth = throttle_reversing_kick * ( TRKD + (float)THROTTLE_REVERSING_DEADTIME ) / TRKD;
	throttle_reversing_kick_decrement = throttle_reversing_kick_sawtooth * (float)LOOPTIME / ( TRKD + (float)THROTTLE_REVERSING_DEADTIME );
	prevent_motor_filtering_state = 1;
}



#if defined(TURTLE_MODE)

bool gTurtleModeActive;	// global so update_osd() in silverlite.cpp can detect if it is active
static float orig_idle_offset = -1.0f;

//------------------------------------------------------------------------------
static void enterTurtleMode()
{
	if (!gTurtleModeActive)
	{
		gTurtleModeActive = true;

		// Set pwm direction to be reverse
		pwm_set_direction(false);

		if (orig_idle_offset < 0)
		{
			orig_idle_offset = idle_offset;
		}
	}
}

//------------------------------------------------------------------------------
static void exitTurtleMode()
{
	if (gTurtleModeActive)
	{
		gTurtleModeActive = false;

		// Set pwm direction to be normal
		pwm_set_direction(true);

		if (orig_idle_offset > 0)
		{
			idle_offset = orig_idle_offset;
		}
	}
}

#endif

#if defined(MAX_THROTTLE_TO_ARM) || defined(TURTLE_MODE)
static bool armed;

//------------------------------------------------------------------------------
// Checks to see if global "armed" state should be changed by examining 
// aux[THROTTLE_KILL_SWITCH]. If so it will also check if turtle mode should be
// enabled/disabled as necessary.
// Returns true if global "armed" state is true
static bool checkArmingState()
{
	char currArmedState = !aux[THROTTLE_KILL_SWITCH];

	// Values within aux[] should always be 0 or 1, 
	// so comparing such a value against 0x88 lets us know if it was valid or not
	static char lastArmedState = 0x88;	
	if (lastArmedState == 0x88)
	{
		lastArmedState = currArmedState;
	}

	if (currArmedState != lastArmedState)
	{
		lastArmedState = currArmedState;
		if (currArmedState)
		{
#if defined(MAX_THROTTLE_TO_ARM)			
			// Toggled to armed state. Check throttle value, if not close enough
			// to zero then ignore the switch
			if (rx[ 3 ] < MAX_THROTTLE_TO_ARM)
#endif			
			{
				// It is safe to arm
				armed = true;

#if defined(TURTLE_MODE)
				if (aux[TURTLE_MODE])
				{
					enterTurtleMode();
				}
#endif				
			}
		}
		else
		{
			// Toggled to disarmed state (kill switch was activated)
			if (armed)
			{
				armed = false;

#if defined(TURTLE_MODE)
				if (gTurtleModeActive)
				{
					exitTurtleMode();
				}
#endif				
			}
		}
	}
	return armed;
}
#endif


void control( bool send_motor_values )
{
	// rates / expert mode
	float rate_multiplier = 1.0f;

	if ( aux[ RATES ] ) {
	} else {
		rate_multiplier = LOW_RATES_MULTI;
	}

#ifdef INVERTED_ENABLE
	const bool motor_direction_changed = ( aux[ FN_INVERTED ] && pwmdir != REVERSE ) || ( ! aux[ FN_INVERTED ] && pwmdir != FORWARD );
	if ( motor_direction_changed ) {
		ierror[ 0 ] = ierror[ 1 ] = ierror[ 2 ] = 0.0f;
		throttle_hpf_reset( 200 ); // ms
#ifdef THROTTLE_REVERSING_KICK
		throttle_kick( THROTTLE_REVERSING_KICK );
#endif
	}

	if ( aux[ FN_INVERTED ] ) {
		pwmdir = REVERSE;
	} else {
		pwmdir = FORWARD;
	}
#endif

	for ( int i = 0; i < 3; ++i ) {
		rxcopy[ i ] = rx[ i ];

#ifdef STICKS_DEADBAND
		if ( fabsf( rxcopy[ i ] ) <= (float)STICKS_DEADBAND ) {
			rxcopy[ i ] = 0.0f;
		} else {
			if ( rxcopy[ i ] > 0.0f ) {
				rxcopy[ i ] = ( rxcopy[ i ] - (float)STICKS_DEADBAND ) * ( 1.0f + (float)STICKS_DEADBAND );
			} else {
				rxcopy[ i ] = ( rxcopy[ i ] + (float)STICKS_DEADBAND ) * ( 1.0f + (float)STICKS_DEADBAND );
			}
		}
#endif // STICKS_DEADBAND
	}

	rxcopy[ 3 ] = rx[ 3 ]; // throttle

	const float mixRangeLimit = MIX_RANGE_LIMIT;
	rxcopy[ 3 ] *= mixRangeLimit;

#ifdef RX_SMOOTHING
	static float rxsmooth[ 4 ];
	static float lastRXcopy[ 4 ];
	static float stepRX[ 4 ];
	static int countRX[ 4 ];
	for ( int i = 0; i < 4; ++i ) {
		if ( rxcopy[ i ] != lastRXcopy[ i ] ) {
			const int step_count = packet_period / LOOPTIME; // Spread it evenly over e.g. 5 ms
			stepRX[ i ] = ( rxcopy[ i ] - lastRXcopy[ i ] ) / step_count;
			countRX[ i ] = step_count;
			rxsmooth[ i ] = lastRXcopy[ i ];
			lastRXcopy[ i ] = rxcopy[ i ];
		}
		if ( countRX[ i ] > 0 ) {
			--countRX[ i ];
			rxsmooth[ i ] += stepRX[ i ];
			rxcopy[ i ] = rxsmooth[ i ];
		} else {
			rxsmooth[ i ] = rxcopy[ i ];
		}
	}
#endif

	pid_precalc();

	// flight control

	float rx_roll = rxcopy[ 0 ];
	float rx_pitch = rxcopy[ 1 ];
	float rx_yaw = rxcopy[ 2 ];
	rx_roll *= rate_multiplier;
	rx_pitch *= rate_multiplier;
	rx_yaw *= rate_multiplier;
#ifdef LEVELMODE
	if ( aux[ LEVELMODE ] ) { // level mode
		extern float angleerror[];
		extern float errorvect[]; // level mode angle error calculated by stick_vector.c
		extern float GEstG[ 3 ]; // gravity vector for yaw feedforward
		float yawerror[ 3 ] = { 0 }; // yaw rotation vector

		// calculate roll / pitch error
		stick_vector( rx_roll, rx_pitch );

		float yawrate = rx_yaw * (float)MAX_RATEYAW * DEGTORAD;
		// apply yaw from the top of the quad
		yawerror[ 0 ] = GEstG[ 1 ] * yawrate;
		yawerror[ 1 ] = -GEstG[ 0 ] * yawrate;
		yawerror[ 2 ] = GEstG[ 2 ] * yawrate;

		// pitch and roll
		for ( int i = 0; i <= 1; ++i ) {
			angleerror[ i ] = errorvect[ i ];
			error[ i ] = angle_pid( i ) + yawerror[ i ] - gyro[ i ];
		}
		// yaw
		error[ 2 ] = yawerror[ 2 ] - gyro[ 2 ];

		// Set ierror to zero, otherwise it builds up and causes bounce back.
		ierror[ 0 ] = 0.0f; ierror[ 1 ] = 0.0f;

		// Nice for BB logging fun. Not needed otherwise.
		for ( int i = 0; i < 3; ++i ) {
			 setpoint[ i ] = error[ i ] + gyro[ i ];
		}
	} else
#endif // LEVELMODE
	{ // rate mode
		setpoint[ 0 ] = rx_roll * (float)MAX_RATE * DEGTORAD;
		setpoint[ 1 ] = rx_pitch * (float)MAX_RATE * DEGTORAD;
		setpoint[ 2 ] = rx_yaw * (float)MAX_RATEYAW * DEGTORAD;

		for ( int i = 0; i < 3; ++i ) {
			error[ i ] = setpoint[ i ] - gyro[ i ];
		}
	}

#ifdef PID_ROTATE_ERRORS
	rotateErrors();
#endif

	pid( 0 );
	pid( 1 );
	pid( 2 );

	static unsigned long motors_failsafe_time = 1;
	bool motors_failsafe = false;
	if ( failsafe ) {
		if ( motors_failsafe_time == 0 ) {
			motors_failsafe_time = gettime();
		}
		if ( gettime() - motors_failsafe_time > MOTORS_FAILSAFETIME ) {
			motors_failsafe = true; // MOTORS_FAILSAFETIME after failsafe we turn off the motors.
		}
	} else {
		motors_failsafe_time = 0;
	}

	// Prevent startup if the TX is turned on with the THROTTLE_KILL_SWITCH not activated:
	static bool tx_just_turned_on = true;
	bool prevent_start = false;
	if ( tx_just_turned_on ) {
		if ( ! aux[ THROTTLE_KILL_SWITCH ] ) {
			prevent_start = true;
		} else {
			tx_just_turned_on = false;
		}
	} else {
		// if ( motors_failsafe ) {
		// 	tx_just_turned_on = true;
		// }
	}

#if defined(MAX_THROTTLE_TO_ARM) || defined(TURTLE_MODE)
	if (!checkArmingState())
	{
		prevent_start = true;
	}
#endif

	if ( motors_failsafe || aux[ THROTTLE_KILL_SWITCH ] || prevent_start ) {
		onground = 1; // This stops the motors.
		thrsum = 0.0f;
		mixmax = 0.0f;

		static int count;
		if ( count > 0 ) {
			--count;
			if ( send_motor_values ) {
				for ( int i = 0; i < 4; ++i ) {
					pwm_set( i, 0 );
				}
			}
		} else {
			// Call motorbeep() only every millisecond, otherwise the beeps get slowed down by the ESC.
			count = 10000 / LOOPTIME - 1;
#ifdef MOTOR_BEEPS
	#ifndef MOTOR_BEEPS_CHANNEL
		#define MOTOR_BEEPS_CHANNEL CH_OFF
	#endif
				motorbeep( motors_failsafe, MOTOR_BEEPS_CHANNEL );
#endif
		}
	} else { // motors on - normal flight
#ifdef THROTTLE_STARTUP_KICK
		if ( onground == 1 ) {
			throttle_kick( THROTTLE_STARTUP_KICK ); // Some startup kick.
		}
#endif // THROTTLE_STARTUP_KICK

		onground = 0;

		float throttle = rxcopy[ 3 ];

#ifdef THROTTLE_VOLTAGE_COMPENSATION
		throttle *= 4.2f / vbattfilt_corr;
		if ( throttle > 1.0f ) {
			throttle = 1.0f;
		}
#endif // THROTTLE_VOLTAGE_COMPENSATION

#ifdef THROTTLE_TRANSIENT_COMPENSATION_FACTOR
		const float throttle_boost = throttle_hpf( throttle ); // Keep the HPF call in the loop to keep its state updated.
		extern bool lowbatt;
		if ( aux[ RATES ] && ! lowbatt ) {
			throttle += (float)THROTTLE_TRANSIENT_COMPENSATION_FACTOR * throttle_boost;
			if ( throttle < 0.0f ) {
				throttle = 0.0f;
			} else if ( throttle > 1.0f ) {
				throttle = 1.0f;
			}
		}
#endif

#ifdef THROTTLE_REVERSING_KICK
		if ( throttle_reversing_kick_sawtooth > 0.0f ) {
			if ( throttle_reversing_kick_sawtooth > throttle_reversing_kick ) {
				throttle = 0.0f;
				pidoutput[ 0 ] = pidoutput[ 1 ] = pidoutput[ 2 ] = 0.0f;
			} else {
				const float transitioning_factor = ( throttle_reversing_kick - throttle_reversing_kick_sawtooth ) / throttle_reversing_kick;
				throttle = throttle_reversing_kick_sawtooth + throttle * transitioning_factor;
				pidoutput[ 0 ] *= transitioning_factor;
				pidoutput[ 1 ] *= transitioning_factor;
				pidoutput[ 2 ] *= transitioning_factor;
				if ( prevent_motor_filtering_state == 1 ) {
					prevent_motor_filtering_state = 2;
				}
			}
			throttle_reversing_kick_sawtooth -= throttle_reversing_kick_decrement;
		}
#endif

#ifdef LVC_LOWER_THROTTLE
		static float throttle_i = 0.0f;
		float throttle_p = 0.0f;

		if ( vbattfilt < (float)LVC_LOWER_THROTTLE_VOLTAGE_RAW ) {
			throttle_p = ( (float)LVC_LOWER_THROTTLE_VOLTAGE_RAW - vbattfilt ) * (float)LVC_LOWER_THROTTLE_KP;
		}
		if ( vbatt_comp < (float)LVC_LOWER_THROTTLE_VOLTAGE ) {
			throttle_p = ( (float)LVC_LOWER_THROTTLE_VOLTAGE - vbatt_comp ) * (float)LVC_LOWER_THROTTLE_KP;
		}
		if ( throttle_p > 1.0f ) {
			throttle_p = 1.0f;
		}
		if ( throttle_p > 0 ) {
			throttle_i += throttle_p * 0.0001f; // ki
		} else {
			throttle_i -= 0.001f; // ki on release
		}

		if ( throttle_i > 0.5f ) {
			throttle_i = 0.5f;
		} else if ( throttle_i < 0.0f ) {
			throttle_i = 0.0f;
		}

		throttle -= throttle_p + throttle_i;
		if ( throttle < 0.0f ) {
			throttle = 0.0f;
		}
#endif

		bb_throttle = throttle;

#ifdef THRUST_LINEARIZATION
		#define AA_motorCurve (float)THRUST_LINEARIZATION // 0 .. linear, 1 .. quadratic
		const float aa = AA_motorCurve;
		throttle = throttle * ( throttle * aa + 1 - aa ); // invert the motor curve correction applied further below
#endif // THRUST_LINEARIZATION

#ifdef ALTERNATIVE_THRUST_LINEARIZATION
		// not really the inverse of ALTERNATIVE_THRUST_LINEARIZATION below, but a good enough approximation
		const float aa = 1.2f * (float)( ALTERNATIVE_THRUST_LINEARIZATION ); // use <1.2 for less compensation
		const float tr = 1.0f - throttle; // throttle reversed
		throttle = throttle / ( 1.0f + tr * tr * aa ); // compensate throttle for the linearization applied further below
#endif // ALTERNATIVE_THRUST_LINEARIZATION

#ifdef INVERT_YAW_PID
		pidoutput[ 2 ] = -pidoutput[ 2 ];
#endif

		float mix[ 4 ];

#ifdef INVERTED_ENABLE
		if ( pwmdir == REVERSE ) { // inverted flight
			mix[ MOTOR_FR - 1 ] = throttle + pidoutput[ ROLL ] + pidoutput[ PITCH ] - pidoutput[ YAW ]; // FR
			mix[ MOTOR_FL - 1 ] = throttle - pidoutput[ ROLL ] + pidoutput[ PITCH ] + pidoutput[ YAW ]; // FL
			mix[ MOTOR_BR - 1 ] = throttle + pidoutput[ ROLL ] - pidoutput[ PITCH ] + pidoutput[ YAW ]; // BR
			mix[ MOTOR_BL - 1 ] = throttle - pidoutput[ ROLL ] - pidoutput[ PITCH ] - pidoutput[ YAW ]; // BL
		} else
#endif
		{ // normal mixer
			mix[ MOTOR_FR - 1 ] = throttle - pidoutput[ ROLL ] - pidoutput[ PITCH ] + pidoutput[ YAW ]; // FR
			mix[ MOTOR_FL - 1 ] = throttle + pidoutput[ ROLL ] - pidoutput[ PITCH ] - pidoutput[ YAW ]; // FL
			mix[ MOTOR_BR - 1 ] = throttle - pidoutput[ ROLL ] + pidoutput[ PITCH ] - pidoutput[ YAW ]; // BR
			mix[ MOTOR_BL - 1 ] = throttle + pidoutput[ ROLL ] + pidoutput[ PITCH ] + pidoutput[ YAW ]; // BL
		}

#ifdef INVERT_YAW_PID
		// we invert again cause it's used by the pid internally (for limit)
		pidoutput[ 2 ] = -pidoutput[ 2 ];
#endif

#ifdef MIX_SCALING
	#ifdef TRANSIENT_MIX_INCREASING_HZ
		float maxSpeedRxcopy = 0.0f;
		static float lastRxcopy[ 4 ];
		for ( int i = 0; i < 4; ++i ) {
			const float absSpeedRxcopy = fabsf( rxcopy[ i ] - lastRxcopy[ i ] ) / LOOPTIME * 1e6f * 0.1f;
			lastRxcopy[ i ] = rxcopy[ i ];
			if ( absSpeedRxcopy > maxSpeedRxcopy ) {
				maxSpeedRxcopy = absSpeedRxcopy;
			}
		}
		static float transientMixIncreaseLimit;
		lpf( &transientMixIncreaseLimit, maxSpeedRxcopy, ALPHACALC( LOOPTIME, 1e6f / (float)( TRANSIENT_MIX_INCREASING_HZ ) ) );
		if ( transientMixIncreaseLimit > 1.0f ) {
			transientMixIncreaseLimit = 1.0f;
		}
	#endif // TRANSIENT_MIX_INCREASING_HZ

		float minMix = 1000.0f;
		float maxMix = -1000.0f;
		for ( int i = 0; i < 4; ++i ) {
			if ( mix[ i ] < minMix ) {
				minMix = mix[ i ];
			}
			if ( mix[ i ] > maxMix ) {
				maxMix = mix[ i ];
			}
		}
		const float mixRange = maxMix - minMix;
		float reduceAmount = 0.0f;
		if ( mixRange > mixRangeLimit ) {
			const float scale = mixRangeLimit / mixRange;
			for ( int i = 0; i < 4; ++i ) {
				mix[ i ] *= scale;
			}
			minMix *= scale;
			reduceAmount = minMix;
		} else {
			if ( maxMix > mixRangeLimit ) {
				reduceAmount = maxMix - mixRangeLimit;
			} else if ( minMix < 0.0f ) {
				reduceAmount = minMix;
			}
		}
	#ifdef ALLOW_MIX_INCREASING
		if ( reduceAmount != 0.0f ) {
	#else
		if ( reduceAmount > 0.0f ) {
	#endif // ALLOW_MIX_INCREASING
	#ifdef TRANSIENT_MIX_INCREASING_HZ
			if ( reduceAmount < -transientMixIncreaseLimit &&
				mixmax > idle_offset + 0.1f ) // Do not apply the limit on idling (e.g. after throttle punches) to prevent from slow wobbles.
			{
				reduceAmount = -transientMixIncreaseLimit;
			}
	#endif // TRANSIENT_MIX_INCREASING_HZ
			for ( int i = 0; i < 4; ++i ) {
				mix[ i ] -= reduceAmount;
			}
		}
#endif // MIX_SCALING

		thrsum = 0.0f;
		mixmax = 0.0f;

#if defined(TURTLE_MODE)
		// If turtle mode is active, precompute some things we'll need to apply
		// to each motor channel in the for() loop following this snippet
		// This code is based on Betaflight source code. Specifically the
		// function applyFlipOverAfterCrashModeToMotors() in mixer.c
		float flipPower;
		float signPitch;
		float signRoll;
		if (gTurtleModeActive)
		{
			float vPitch = rxcopy[PITCH];
			float vRoll = rxcopy[ROLL];
			float vPitchAbs = fabsf(vPitch);
			float vRollAbs = fabsf(vRoll);

			signPitch = vPitch < 0 ? +1 : -1;
		 	signRoll = vRoll < 0 ? +1 : -1;

			float length = sqrtf(vPitchAbs*vPitchAbs + vRollAbs*vRollAbs);
			float cosPhi = (vPitchAbs + vRollAbs) / (sqrtf(2.0f) * length);
			const float cosThreshold = sqrtf(3.0f)/2.0f; // cos(PI/6.0f)
			if (cosPhi < cosThreshold) 
			{
				// Enforce either roll or pitch exclusively, if not on diagonal
				if (vRollAbs > vPitchAbs) 
				{
					signPitch = 0;
				}
				else 
				{
					signRoll = 0;
				}
			}

			// Apply a reasonable amount of stick deadband
			#define CRASH_FLIP_STICK_MINF	0.15f
			const float flipStickRange = 1.0f - CRASH_FLIP_STICK_MINF;
			flipPower = length - CRASH_FLIP_STICK_MINF;
			if (flipPower < 0)
			{
				flipPower = 0.0f;
			}
			flipPower /= flipStickRange;
		}
#endif

		for ( int i = 0; i < 4; ++i ) { // For each motor.

#if defined(MOTORS_TO_THROTTLE) || defined(MOTORS_TO_THROTTLE_MODE)

			static float orig_idle_offset = 0.0f;
#if defined(MOTORS_TO_THROTTLE_MODE) && !defined(MOTORS_TO_THROTTLE)
			if ( orig_idle_offset == 0 ) {
				orig_idle_offset = idle_offset;
			}
			if ( aux[ MOTORS_TO_THROTTLE_MODE ] ) {
#endif
				mix[ i ] = throttle;
				if ( throttle > 0 ) {
					idle_offset = orig_idle_offset;
				}
				if ( ( i == MOTOR_FL - 1 && rxcopy[ ROLL ] < 0 && rxcopy[ PITCH ] > 0 ) ||
					( i == MOTOR_BL - 1 && rxcopy[ ROLL ] < 0 && rxcopy[ PITCH ] < 0 ) ||
					( i == MOTOR_FR - 1 && rxcopy[ ROLL ] > 0 && rxcopy[ PITCH ] > 0 ) ||
					( i == MOTOR_BR - 1 && rxcopy[ ROLL ] > 0 && rxcopy[ PITCH ] < 0 ) )
				{
					idle_offset = 0.0f;
					mix[ i ] = fabsf( rxcopy[ ROLL ] * rxcopy[ PITCH ] * rate_multiplier );
#ifdef RPM_FILTER
	#if !defined(USE_SILVERLITE) // We don't implement notify_telemetry_value() with SilverLite-FC
					extern float motor_hz[ 4 ];
					notify_telemetry_value( motor_hz[ i ] );
	#endif					
#endif // RPM_FILTER
				}
				ledcommand = true;
#if defined(MOTORS_TO_THROTTLE_MODE) && !defined(MOTORS_TO_THROTTLE)
			} else {
				idle_offset = orig_idle_offset;
			}
#endif

#endif // defined(MOTORS_TO_THROTTLE) || defined(MOTORS_TO_THROTTLE_MODE)

#if defined(TURTLE_MODE)
			if (gTurtleModeActive)
			{
				ledcommand = 1;
				idle_offset = 0.0f;

				float power;
				switch (i)
				{
					case MOTOR_FR - 1:
						power = -1.0f*signPitch + -1.0f*signRoll;
						break;
					case MOTOR_FL - 1:
						power = -1.0f*signPitch + +1.0f*signRoll;
						break;
					case MOTOR_BR - 1:
						power = +1.0f*signPitch + -1.0f*signRoll;
						break;
					case MOTOR_BL - 1:
						power = +1.0f*signPitch + +1.0f*signRoll;
						break;
				}
				if (power < 0.0f)
				{
					power = 0.0f;
				}

				power = power * flipPower;
				if (power > 1.0f)
				{
					power = 1.0f;
				}
				mix[i] = power;
			}
#endif
			if ( mix[ i ] < 0.0f ) {
				mix[ i ] = 0.0f;
			} else if ( mix[ i ] > 1.0f )	{
				mix[ i ] = 1.0f;
			}

			mix[ i ] = idle_offset + mix[ i ] * ( 1.0f - idle_offset ); // maps 0 .. 1 -> idle_offset .. 1

#ifdef THRUST_LINEARIZATION
			// Computationally quite expensive:
			static float a, a_reci, b, b_sq;
			if ( a != AA_motorCurve ) {
				a = AA_motorCurve;
				if ( a > 0.0f ) {
					a_reci = 1 / a;
					b = ( 1 - a ) / ( 2 * a );
					b_sq = b * b;
				}
			}
			if ( mix[ i ] > 0.0f && a > 0.0f ) {
				mix[ i ] = sqrtf( mix[ i ] * a_reci + b_sq ) - b;
			}
#endif

#ifdef ALTERNATIVE_THRUST_LINEARIZATION
			const float mr = 1.0f - mix[ i ]; // mix reversed
			mix[ i ] *= 1.0f + mr * mr * (float)( ALTERNATIVE_THRUST_LINEARIZATION );
#endif // ALTERNATIVE_THRUST_LINEARIZATION

			if ( send_motor_values ) {
				pwm_set( i, mix[ i ] );
			}
			bb_mix[ i ] = mix[ i ];

			thrsum += mix[ i ];
			if ( mixmax < mix[ i ] ) {
				mixmax = mix[ i ];
			}
		} // For each motor.

		if ( prevent_motor_filtering_state == 2 ) {
	 		prevent_motor_filtering_state = 0;
	 	}
		thrsum = thrsum / 4.0f;
	} // end motors on
}
