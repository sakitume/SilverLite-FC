#include "config.h"
#include "filter.h"
#include "util.h"

extern char aux[ AUXNUMBER ];
extern float aux_analog[ 2 ];
extern float rxcopy[ 4 ];


typedef struct FilterLPF2Coeff_s {
	float two_one_minus_alpha;
	float one_minus_alpha_sqr;
	float alpha_sqr;
} FilterLPF2Coeff_t;

typedef struct FilterLPF2_s {
	float last_out;
	float last_out2;
	int holdoff_steps;
} FilterLPF2_t;

static float filter_lpf2_step( FilterLPF2_t * filter, FilterLPF2Coeff_t * coeff, float in )
{
	if ( filter->holdoff_steps > 0 ) {
		--filter->holdoff_steps;
		return 0.0f;
	}

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
	const float one_minus_alpha = FILTERCALC( LOOPTIME, 1e6f / filter_Hz );
	coeff->one_minus_alpha_sqr = one_minus_alpha * one_minus_alpha;
	coeff->two_one_minus_alpha = 2 * one_minus_alpha;
	const float alpha = 1 - one_minus_alpha;
	coeff->alpha_sqr = alpha * alpha;
}

static void filter_lpf2_reset( FilterLPF2_t * filter, int holdoff_time_ms )
{
	filter->last_out = filter->last_out2 = 0.0f;
	filter->holdoff_steps = holdoff_time_ms * 1000 / LOOPTIME;
}


// Gyro

#ifdef GYRO_FILTER_NONE
float gyro_filter( float in, int num )
{
	return in;
}
#endif // GYRO_FILTER_NONE


#ifdef BIQUAD_NOTCH_HZ

static float b0, b1, b2, a0, a1, a2;

static void notch_init( float filter_Hz, float filter_Q )
{
	// setup variables
	const float omega = 2.0f * PI_F * filter_Hz * LOOPTIME * 1e-6f;
	const float sn = fastsin( omega );
	const float cs = fastcos( omega );
	const float alpha = sn / ( 2.0f * filter_Q );

	// notch coefficients
	b0 = 1;
	b1 = -2 * cs;
	b2 = 1;
	a0 = 1 + alpha;
	a1 = -2 * cs;
	a2 = 1 - alpha;

	// precompute the coefficients
	b0 /= a0;
	b1 /= a0;
	b2 /= a0;
	a1 /= a0;
	a2 /= a0;
}

float gyro_filter( float input, int num )
{
	static float notch_Hz, notch_Q;
	if ( notch_Hz != BIQUAD_NOTCH_HZ || notch_Q != BIQUAD_NOTCH_Q ) {
		notch_Hz = BIQUAD_NOTCH_HZ;
		notch_Q = BIQUAD_NOTCH_Q;
		notch_init( notch_Hz, notch_Q );
	}
	static float x1[ 3 ], x2[ 3 ];
#if 1
	static float y1[ 3 ], y2[ 3 ];
	// Direct Form I
	const float result = b0 * input + b1 * x1[ num ] + b2 * x2[ num ] - a1 * y1[ num ] - a2 * y2[ num ];
	x2[ num ] = x1[ num ];
	x1[ num ] = input;
	y2[ num ] = y1[ num ];
	y1[ num ] = result;
	return result;
#else
	// Direct Form II
	const float result = b0 * input + x1[ num ];
	x1[ num ] = b1 * input - a1 * result + x2[ num ];
	x2[ num ] = b2 * input - a2 * result;
	return result;
#endif
}

#endif // BIQUAD_NOTCH_HZ


#ifdef DYNAMIC_LPF_1ST_HZ

static float one_minus_alpha;
static float gyro_lpf_last[ 3 ];

float gyro_filter( float in, int num )
{
	if ( num == 0 ) { // recalculate coeff
		const float throttle = rxcopy[ 3 ];
		const float f_base = DYNAMIC_LPF_1ST_HZ_BASE;
		const float f_max = DYNAMIC_LPF_1ST_HZ_MAX;
		const float throttle_breakpoint = DYNAMIC_LPF_1ST_HZ_THROTTLE;
		float filter_Hz = f_base + throttle / throttle_breakpoint * ( f_max - f_base );
		if ( filter_Hz > f_max ) {
			filter_Hz = f_max;
		} else if ( filter_Hz < f_base ) {
			filter_Hz = f_base;
		}
		one_minus_alpha = FILTERCALC( LOOPTIME, 1e6f / filter_Hz );
	}
	lpf( &gyro_lpf_last[ num ], in, one_minus_alpha );
	return gyro_lpf_last[ num ];
}

#endif // DYNAMIC_LPF_1ST_HZ


#ifdef DYNAMIC_LPF_2ND_HZ

static FilterLPF2Coeff_t gyro_lpf2_coeff;
static FilterLPF2_t gyro_lpf2[ 3 ];

float gyro_filter( float in, int num )
{
	if ( num == 0 ) { // recalculate coeffs
		const float throttle = rxcopy[ 3 ];
		const float f_base = DYNAMIC_LPF_2ND_HZ_BASE;
		const float f_max = DYNAMIC_LPF_2ND_HZ_MAX;
		const float throttle_breakpoint = DYNAMIC_LPF_2ND_HZ_THROTTLE;
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
	return filter_lpf2_step( &gyro_lpf2[ num ], &gyro_lpf2_coeff, in );
}

#endif // DYNAMIC_LPF_2ND_HZ


// D-Term

static FilterLPF2Coeff_t dterm_lpf2_coeff;
static FilterLPF2_t dterm_lpf2[ 3 ];

float dterm_filter( float in, int num )
{
	if ( num == 0 ) { // recalculate coeffs
		const float throttle = rxcopy[ 3 ];
		const float f_base = DYNAMIC_DTERM_LPF_2ND_HZ_BASE;
		const float f_max = DYNAMIC_DTERM_LPF_2ND_HZ_MAX;
		const float throttle_breakpoint = DYNAMIC_DTERM_LPF_2ND_HZ_THROTTLE;
		float filter_Hz = f_base + throttle / throttle_breakpoint * ( f_max - f_base );
		if ( filter_Hz > f_max ) {
			filter_Hz = f_max;
		} else if ( filter_Hz < f_base ) {
			filter_Hz = f_base;
		}
		filter_lpf2_coeff( &dterm_lpf2_coeff, filter_Hz );
	}
	return filter_lpf2_step( &dterm_lpf2[ num ], &dterm_lpf2_coeff, in );
}

void dterm_filter_reset( int holdoff_time_ms )
{
	filter_lpf2_reset( &dterm_lpf2[ 0 ], holdoff_time_ms );
	filter_lpf2_reset( &dterm_lpf2[ 1 ], holdoff_time_ms );
	filter_lpf2_reset( &dterm_lpf2[ 2 ], holdoff_time_ms );
}


// 16 Hz hpf filter for throttle boost
typedef struct FilterHPF_s {
	float last_lpf;
	int holdoff_steps;
} FilterHPF_t;

static FilterHPF_t throttle_hpf1;

float throttle_hpf( float in )
{
	lpf( &throttle_hpf1.last_lpf, in, FILTERCALC( LOOPTIME, 1e6f / 16.0f ) ); // 16 Hz

	if ( throttle_hpf1.holdoff_steps > 0 ) {
		--throttle_hpf1.holdoff_steps;
		return 0.0f;
	}

	return in - throttle_hpf1.last_lpf; // HPF = input - average_input
}

void throttle_hpf_reset( int holdoff_time_ms )
{
	throttle_hpf1.last_lpf = 0.0f;
	throttle_hpf1.holdoff_steps = holdoff_time_ms * 1000 / LOOPTIME;
}
