#include "config.h"
#include "filter.h"
#include "math.h"
#include "util.h"
#include "stdbool.h"

extern char aux[ AUXNUMBER ];
extern float aux_analog[ 2 ];
extern float rxcopy[ 4 ];


void lpf( float * out, float in, float alpha )
{
	*out += alpha * ( in - *out );
}

void lpf_hz( float * out, float in, float f_hz )
{
	*out += ALPHACALC( LOOPTIME, 1e6f / f_hz ) * ( in - *out );
}

// Biquad

typedef struct FilterBiquadCoeff_s {
	float b0, b1, b2, a1, a2, f_Hz;
} FilterBiquadCoeff_t;

typedef struct FilterBiquad_s {
	float x1, x2;
#ifdef USE_DIRECT_FORM_I
	float y1, y2;
#endif
} FilterBiquad_t;

static void filter_notch_coeff( FilterBiquadCoeff_t * coeff, float filter_Hz, float filter_Q )
{
	if ( filter_Hz == 0.0f || filter_Q == 0.0f ) {
		return;
	}

	// setup variables
	const float omega = 2.0f * PI_F * filter_Hz * LOOPTIME * 1e-6f;
	const float sn = sin_approx( omega );
	const float cs = cos_approx( omega );
	const float alpha = sn / ( 2.0f * filter_Q );

	// notch coefficients
	coeff->b0 = 1;
	coeff->b1 = -2 * cs;
	coeff->b2 = 1;
	const float a0 = 1 + alpha;
	coeff->a1 = -2 * cs;
	coeff->a2 = 1 - alpha;

	// precompute the coefficients
	coeff->b0 /= a0;
	coeff->b1 /= a0;
	coeff->b2 /= a0;
	coeff->a1 /= a0;
	coeff->a2 /= a0;

	// filter frequency for the above coefficients
	coeff->f_Hz = filter_Hz;
}

static void filter_peak_coeff( FilterBiquadCoeff_t * coeff, float filter_Hz, float filter_Q, float filter_gain )
{
	if ( filter_Hz == 0.0f || filter_Q == 0.0f || filter_gain == 0.0f ) {
		return;
	}

	// setup variables
	const float gain_abs = sqrtf( filter_gain );
	const float omega = 2.0f * PI_F * filter_Hz * LOOPTIME * 1e-6f;
	const float sn = sin_approx( omega );
	const float cs = cos_approx( omega );
	const float alpha = sn / ( 2.0f * filter_Q );

	// peak coefficients
	coeff->b0 = 1 + alpha * gain_abs;
	coeff->b1 = -2 * cs;
	coeff->b2 = 1 - alpha * gain_abs;
	const float a0 = 1 + alpha / gain_abs;
	coeff->a1 = -2 * cs;
	coeff->a2 = 1 - alpha / gain_abs;

	// precompute the coefficients
	coeff->b0 /= a0;
	coeff->b1 /= a0;
	coeff->b2 /= a0;
	coeff->a1 /= a0;
	coeff->a2 /= a0;

	// filter frequency for the above coefficients
	coeff->f_Hz = filter_Hz;
}

static void filter_bessel_coeff( FilterBiquadCoeff_t * coeff, float filter_Hz )
{
	if ( filter_Hz == 0.0f ) {
		return;
	}

	// Bessel coefficients from Beads project.
	const float omega_halve = PI_F * filter_Hz * LOOPTIME * 1e-6f * 0.5f; // 0.5f is there to empirically match with lpf2.
	const float tg = sin_approx( omega_halve ) / cos_approx( omega_halve );
	coeff->b2 = coeff->b0 = 3 * tg * tg;
	coeff->b1 = 2 * coeff->b0;
	const float a0 = 1 + 3 * tg + coeff->b0;
	coeff->a1 = -2 + coeff->b1;
	coeff->a2 = 1 - 3 * tg + coeff->b0;

	// precompute the coefficients
	coeff->b0 /= a0;
	coeff->b1 /= a0;
	coeff->b2 /= a0;
	coeff->a1 /= a0;
	coeff->a2 /= a0;

	// filter frequency for the above coefficients
	coeff->f_Hz = filter_Hz;
}

static float filter_biquad_step( FilterBiquad_t * filter, FilterBiquadCoeff_t * coeff, float input )
{
#ifdef USE_DIRECT_FORM_I
	// Direct Form I
	const float result = coeff->b0 * input + coeff->b1 * filter->x1 + coeff->b2 * filter->x2 - coeff->a1 * filter->y1 - coeff->a2 * filter->y2;
	filter->x2 = filter->x1;
	filter->x1 = input;
	filter->y2 = filter->y1;
	filter->y1 = result;
	return result;
#else
	// Direct Form II
	const float result = coeff->b0 * input + filter->x1;
	filter->x1 = coeff->b1 * input - coeff->a1 * result + filter->x2;
	filter->x2 = coeff->b2 * input - coeff->a2 * result;
	return result;
#endif
}


// LPF2

typedef struct FilterLPF2Coeff_s {
	float two_one_minus_alpha;
	float one_minus_alpha_sqr;
	float alpha_sqr;
} FilterLPF2Coeff_t;

typedef struct FilterLPF2_s {
	float last_out;
	float last_out2;
} FilterLPF2_t;

static float filter_lpf2_step( FilterLPF2_t * filter, FilterLPF2Coeff_t * coeff, float in )
{
	const float ans =
		in * coeff->alpha_sqr
		+ coeff->two_one_minus_alpha * filter->last_out
		- coeff->one_minus_alpha_sqr * filter->last_out2;
	filter->last_out2 = filter->last_out;
	filter->last_out = ans;

	return ans;
}

static void filter_lpf2_coeff( FilterLPF2Coeff_t * coeff, float filter_Hz )
{
	if ( filter_Hz == 0.0f ) {
		return;
	}

	const float alpha = ALPHACALC( LOOPTIME, 1e6f / filter_Hz );
	const float one_minus_alpha = 1 - alpha;
	coeff->one_minus_alpha_sqr = one_minus_alpha * one_minus_alpha;
	coeff->two_one_minus_alpha = 2 * one_minus_alpha;
	coeff->alpha_sqr = alpha * alpha;
}


// Gyro

#ifdef RPM_FILTER

float rpm_filter( float input, int axis )
{
	#define RPM_FILTER_HARMONICS 3
	extern float motor_hz[ 4 ]; // drv_dshot_bidir.c
	static FilterBiquadCoeff_t gyro_notch_coeff[ 4 ][ RPM_FILTER_HARMONICS ];
	static FilterBiquad_t gyro_notch[ 3 ][ 4 ][ RPM_FILTER_HARMONICS ];
	float output = input;
	for ( int motor = 0; motor < 4; ++motor ) {
		for ( int harmonic = 0; harmonic < RPM_FILTER_HARMONICS; ++harmonic ) {
			if ( harmonic == 0 || ( RPM_FILTER_2ND_HARMONIC && harmonic == 1 ) || ( RPM_FILTER_3RD_HARMONIC && harmonic == 2 ) ) {
				const float filter_hz_harmonic = motor_hz[ motor ] * ( harmonic + 1 );
				if ( axis == 0 && filter_hz_harmonic >= RPM_FILTER_HZ_MIN ) {
					filter_notch_coeff( &gyro_notch_coeff[ motor ][ harmonic ], filter_hz_harmonic, RPM_FILTER_Q );
				}
				const float filtered = filter_biquad_step( &gyro_notch[ axis ][ motor ][ harmonic ], &gyro_notch_coeff[ motor ][ harmonic ], output );
				if ( gyro_notch_coeff[ motor ][ harmonic ].f_Hz >= RPM_FILTER_HZ_MIN ) {
					output = filtered;
				}
			}
		}
	}
	return output;
}

#endif // RPM_FILTER


#ifdef BIQUAD_NOTCH_A_HZ

float notch_a_filter( float input, int num )
{
	static FilterBiquadCoeff_t gyro_notch_coeff;
	static FilterBiquad_t gyro_notch[ 3 ];
	static float notch_Hz, notch_Q;
	if ( notch_Hz != (float)BIQUAD_NOTCH_A_HZ || notch_Q != (float)BIQUAD_NOTCH_A_Q ) {
		notch_Hz = BIQUAD_NOTCH_A_HZ;
		notch_Q = BIQUAD_NOTCH_A_Q;
		filter_notch_coeff( &gyro_notch_coeff, notch_Hz, notch_Q );
	}
	return filter_biquad_step( &gyro_notch[ num ], &gyro_notch_coeff, input );
}

#endif // BIQUAD_NOTCH_A_HZ


#ifdef BIQUAD_NOTCH_B_HZ

float notch_b_filter( float input, int num )
{
	static FilterBiquadCoeff_t gyro_notch_coeff;
	static FilterBiquad_t gyro_notch[ 3 ];
	static float notch_Hz, notch_Q;
	if ( notch_Hz != (float)BIQUAD_NOTCH_B_HZ || notch_Q != (float)BIQUAD_NOTCH_B_Q ) {
		notch_Hz = BIQUAD_NOTCH_B_HZ;
		notch_Q = BIQUAD_NOTCH_B_Q;
		filter_notch_coeff( &gyro_notch_coeff, notch_Hz, notch_Q );
	}
	return filter_biquad_step( &gyro_notch[ num ], &gyro_notch_coeff, input );
}

#endif // BIQUAD_NOTCH_B_HZ


#ifdef BIQUAD_NOTCH_C_HZ

float notch_c_filter( float input, int num )
{
	static FilterBiquadCoeff_t gyro_notch_coeff;
	static FilterBiquad_t gyro_notch[ 3 ];
	static float notch_Hz, notch_Q;
	if ( notch_Hz != (float)BIQUAD_NOTCH_C_HZ || notch_Q != (float)BIQUAD_NOTCH_C_Q ) {
		notch_Hz = BIQUAD_NOTCH_C_HZ;
		notch_Q = BIQUAD_NOTCH_C_Q;
		filter_notch_coeff( &gyro_notch_coeff, notch_Hz, notch_Q );
	}
	return filter_biquad_step( &gyro_notch[ num ], &gyro_notch_coeff, input );
}

#endif // BIQUAD_NOTCH_C_HZ


#ifdef BIQUAD_AUTO_NOTCH

float auto_notch_Hz = 0.0f;

float auto_notch_filter( float input, int num )
{
	static FilterBiquadCoeff_t gyro_notch_coeff;
	static FilterBiquad_t gyro_notch[ 3 ];
	static float notch_Hz, notch_Q;
	if ( notch_Hz != auto_notch_Hz || notch_Q != (float)BIQUAD_AUTO_NOTCH_Q ) {
		notch_Hz = auto_notch_Hz;
		notch_Q = BIQUAD_AUTO_NOTCH_Q;
		filter_notch_coeff( &gyro_notch_coeff, notch_Hz, notch_Q );
	}
	if ( notch_Hz != 0.0f ) {
		return filter_biquad_step( &gyro_notch[ num ], &gyro_notch_coeff, input );
	} else {
		return input;
	}
}

#endif // BIQUAD_AUTO_NOTCH


#ifdef BIQUAD_SDFT_NOTCH

float sdft_notch_Hz[ SDFT_AXES * 2 ];

float sdft_notch_filter( float input, int num )
{
	static FilterBiquadCoeff_t gyro_notch_coeff[ SDFT_AXES * 2 ];
	static FilterBiquad_t gyro_notch[ SDFT_AXES * 2 ];
	static float notch_Hz[ SDFT_AXES * 2 ];
	if ( notch_Hz[ num ] != sdft_notch_Hz[ num ] ) {
		notch_Hz[ num ] = sdft_notch_Hz[ num ];
		float Q = sdft_notch_Hz[ num ] / 20.0f; // 20 Hz bandwidth
		Q = Q > 6.0f ? 6.0f : Q; // Limit Q to 6, otherwise the settling time gets too long.
		filter_notch_coeff( &gyro_notch_coeff[ num ], sdft_notch_Hz[ num ], Q );
		// filter_peak_coeff( &gyro_notch_coeff[ num ], sdft_notch_Hz[ num ], Q, 0.2f );
	}
	return filter_biquad_step( &gyro_notch[ num ], &gyro_notch_coeff[ num ], input );
}

#endif // BIQUAD_SDFT_NOTCH


#ifdef BIQUAD_PEAK_HZ

float peak_filter( float input, int num )
{
	static FilterBiquadCoeff_t gyro_peak_coeff;
	static FilterBiquad_t gyro_peak[ 3 ];
	static float peak_Hz, peak_Q, peak_gain;
	if ( peak_Hz != (float)BIQUAD_PEAK_HZ || peak_Q != (float)BIQUAD_PEAK_Q || peak_gain != (float)BIQUAD_PEAK_GAIN ) {
		peak_Hz = BIQUAD_PEAK_HZ;
		peak_Q = BIQUAD_PEAK_Q;
		peak_gain = BIQUAD_PEAK_GAIN;
		filter_peak_coeff( &gyro_peak_coeff, peak_Hz, peak_Q, peak_gain );
	}
	return filter_biquad_step( &gyro_peak[ num ], &gyro_peak_coeff, input );
}

#endif // BIQUAD_PEAK_HZ


#ifdef GYRO_LPF_1ST_HZ_BASE

static float gyro_lpf_alpha;
static float gyro_lpf_last[ 3 ];

float gyro_lpf_filter( float in, int num )
{
	static bool base_and_max_differ = true; // initialize with true so we enter at least once
	static float f_base, f_max;
	const bool base_or_max_changed = f_base != GYRO_LPF_1ST_HZ_BASE || f_max != GYRO_LPF_1ST_HZ_MAX;
	if ( ( base_and_max_differ || base_or_max_changed ) && num == 0 ) { // recalculate coeffs
		base_and_max_differ = GYRO_LPF_1ST_HZ_BASE != GYRO_LPF_1ST_HZ_MAX;
		const float throttle = rxcopy[ 3 ];
		f_base = GYRO_LPF_1ST_HZ_BASE;
		f_max = GYRO_LPF_1ST_HZ_MAX;
		const float throttle_breakpoint = GYRO_LPF_1ST_HZ_THROTTLE;
		float filter_Hz = f_base + throttle / throttle_breakpoint * ( f_max - f_base );
		if ( filter_Hz > f_max ) {
			filter_Hz = f_max;
		} else if ( filter_Hz < f_base ) {
			filter_Hz = f_base;
		}
		if ( filter_Hz != 0.0f ) {
			gyro_lpf_alpha = ALPHACALC( LOOPTIME, 1e6f / filter_Hz );
		}
	}
	lpf( &gyro_lpf_last[ num ], in, gyro_lpf_alpha );
	return gyro_lpf_last[ num ];
}

#endif // GYRO_LPF_1ST_HZ_BASE


#ifdef GYRO_LPF_2ND_HZ_BASE

static FilterLPF2Coeff_t gyro_lpf2_coeff;
static FilterLPF2_t gyro_lpf2[ 3 ];

float gyro_lpf2_filter( float in, int num )
{
	static bool base_and_max_differ = true; // initialize with true so we enter at least once
	static float f_base, f_max;
	const bool base_or_max_changed = f_base != GYRO_LPF_2ND_HZ_BASE || f_max != GYRO_LPF_2ND_HZ_MAX;
	if ( ( base_and_max_differ || base_or_max_changed ) && num == 0 ) { // recalculate coeffs
		base_and_max_differ = GYRO_LPF_2ND_HZ_BASE != GYRO_LPF_2ND_HZ_MAX;
		const float throttle = rxcopy[ 3 ];
		f_base = GYRO_LPF_2ND_HZ_BASE;
		f_max = GYRO_LPF_2ND_HZ_MAX;
		const float throttle_breakpoint = GYRO_LPF_2ND_HZ_THROTTLE;
		if ( throttle_breakpoint != 0.0f ) {
#if 1
			float filter_Hz = f_base + throttle / throttle_breakpoint * ( f_max - f_base );
#else
			// const float absyaw = fabsf( rxcopy[ 2 ] );
			const float absyaw = fabsf( gyro[ 2 ] / ( (float)MAX_RATEYAW * DEGTORAD ) );
			float filter_Hz = f_base + ( throttle / throttle_breakpoint + 0.5f * absyaw ) * ( f_max - f_base );
#endif
			if ( filter_Hz > f_max ) {
				filter_Hz = f_max;
			} else if ( filter_Hz < f_base ) {
				filter_Hz = f_base;
			}
			filter_lpf2_coeff( &gyro_lpf2_coeff, filter_Hz );
		}
	}
	return filter_lpf2_step( &gyro_lpf2[ num ], &gyro_lpf2_coeff, in );
}

#endif // GYRO_LPF_2ND_HZ


// D-Term

#ifdef DTERM_LPF_2ND_HZ_BASE

static FilterBiquadCoeff_t dterm_bessel_coeff;
static FilterBiquad_t dterm_bessel[ 3 ];

static FilterLPF2Coeff_t dterm_lpf2_coeff;
static FilterLPF2_t dterm_lpf2[ 3 ];

float dterm_filter( float in, int num )
{
	static bool base_and_max_differ = true; // initialize with true so we enter at least once
	static float f_base, f_max;
	const bool base_or_max_changed = f_base != DTERM_LPF_2ND_HZ_BASE || f_max != DTERM_LPF_2ND_HZ_MAX;
	if ( ( base_and_max_differ || base_or_max_changed ) && num == 0 ) { // recalculate coeffs
		base_and_max_differ = DTERM_LPF_2ND_HZ_BASE != DTERM_LPF_2ND_HZ_MAX;
		const float throttle = rxcopy[ 3 ];
		f_base = DTERM_LPF_2ND_HZ_BASE;
		f_max = DTERM_LPF_2ND_HZ_MAX;
		const float throttle_breakpoint = DTERM_LPF_2ND_HZ_THROTTLE;
		if ( throttle_breakpoint != 0.0f ) {
			float filter_Hz = f_base + throttle / throttle_breakpoint * ( f_max - f_base );
			if ( filter_Hz > f_max ) {
				filter_Hz = f_max;
			} else if ( filter_Hz < f_base ) {
				filter_Hz = f_base;
			}
#ifdef DTERM_BESSEL_FILTER
			filter_bessel_coeff( &dterm_bessel_coeff, filter_Hz );
#else
			filter_lpf2_coeff( &dterm_lpf2_coeff, filter_Hz );
#endif // DTERM_BESSEL_FILTER
		}
	}
#ifdef DTERM_BESSEL_FILTER
	return filter_biquad_step( &dterm_bessel[ num ], &dterm_bessel_coeff, in );
#else
	return filter_lpf2_step( &dterm_lpf2[ num ], &dterm_lpf2_coeff, in );
#endif // DTERM_BESSEL_FILTER
}

#endif // DTERM_LPF_2ND_HZ_BASE


// 16 Hz hpf filter for throttle boost
typedef struct FilterHPF_s {
	float in_lpf;
	float avg_boost;
	int holdoff_steps;
} FilterHPF_t;

static FilterHPF_t throttle_hpf1;

float throttle_hpf( float in )
{
	if ( throttle_hpf1.holdoff_steps > 0 ) {
		--throttle_hpf1.holdoff_steps;
		throttle_hpf1.in_lpf = in;
		return 0.0f;
	}

	lpf( &throttle_hpf1.in_lpf, in, ALPHACALC( LOOPTIME, 1e6f / 16.0f ) ); // 16 Hz for HPF
	const float boost = in - throttle_hpf1.in_lpf; // HPF = input - average_input
	lpf( &throttle_hpf1.avg_boost, boost, ALPHACALC( LOOPTIME, 1e6f / 8.0f ) ); // 8 Hz for LPF
	return throttle_hpf1.avg_boost;
}

void throttle_hpf_reset( int holdoff_time_ms )
{
	throttle_hpf1.in_lpf = 0.0f;
	throttle_hpf1.avg_boost = 0.0f;
	throttle_hpf1.holdoff_steps = holdoff_time_ms * 1000 / LOOPTIME;
}


#ifdef KALMAN_q

#define KALMAN_WINDOW_SIZE 256

typedef struct Kalman_s {
	// float q; // process noise covariance
	float r; // measurement noise covariance
	float p; // estimation error covariance matrix
	float k; // kalman gain
	float x; // state
	float lastX; // previous state

	float window[ KALMAN_WINDOW_SIZE ];
	float meanSum;
	float varianceSum;
	int index;
} Kalman_t;

static float kalman_step( Kalman_t * filter, float input )
{
	// update variance
	filter->window[ filter->index ] = input;
	filter->meanSum += filter->window[ filter->index ];
	filter->varianceSum += filter->window[ filter->index ] * filter->window[ filter->index ];
	++filter->index;
	if ( filter->index == KALMAN_WINDOW_SIZE ) {
		filter->index = 0;
	}
	filter->meanSum -= filter->window[ filter->index ];
	filter->varianceSum -= filter->window[ filter->index ] * filter->window[ filter->index ];
	const float mean = filter->meanSum / KALMAN_WINDOW_SIZE;
	const float variance = fabsf( filter->varianceSum / KALMAN_WINDOW_SIZE - mean * mean );
	filter->r = sqrtf( variance );

	// project the state ahead using acceleration
	filter->x += filter->x - filter->lastX;
	// update last state
	filter->lastX = filter->x;
	// prediction update
	filter->p += KALMAN_q * 1e-6f;
	// measurement update
	filter->k = filter->p / ( filter->p + filter->r );
	filter->x += filter->k * ( input - filter->x );
	filter->p = ( 1.0f - filter->k ) * filter->p;

	return filter->x;
}

static Kalman_t kalman_lpf[ 4 ];

float kalman_filter( float input, int num )
{
	return kalman_step( &kalman_lpf[ num ], input );
}

void kalman_set( float input, int num )
{
	kalman_lpf[ num ].x = input;
	kalman_lpf[ num ].lastX = input;
}

#endif // KALMAN_q
