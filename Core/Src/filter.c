#include "config.h"
#include "filter.h"
#include "util.h"

extern char aux[ AUXNUMBER ];
extern float aux_analog[ 2 ];
extern float rxcopy[ 4 ];


// Notch

typedef struct FilterNotchCoeff_s {
	float b0, b1, b2, a1, a2;
} FilterNotchCoeff_t;

typedef struct FilterNotch_s {
	float x1, x2, y1, y2;
} FilterNotch_t;

static void filter_notch_coeff( FilterNotchCoeff_t * coeff, float filter_Hz, float filter_Q )
{
	if ( filter_Hz == 0.0f || filter_Q == 0.0f ) {
		return;
	}

	// setup variables
	const float omega = 2.0f * PI_F * filter_Hz * LOOPTIME * 1e-6f;
	const float sn = fastsin( omega );
	const float cs = fastcos( omega );
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
}

float filter_notch_step( FilterNotch_t * filter, FilterNotchCoeff_t * coeff, float input )
{
#if 1
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
	if ( filter_Hz == 0.0f ) {
		return;
	}

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

#ifdef BIQUAD_NOTCH_A_HZ

float notch_a_filter( float input, int num )
{
	static FilterNotchCoeff_t gyro_notch_coeff;
	static FilterNotch_t gyro_notch[ 3 ];
	static float notch_Hz, notch_Q;
	if ( notch_Hz != BIQUAD_NOTCH_A_HZ || notch_Q != BIQUAD_NOTCH_A_Q ) {
		notch_Hz = BIQUAD_NOTCH_A_HZ;
		notch_Q = BIQUAD_NOTCH_A_Q;
		filter_notch_coeff( &gyro_notch_coeff, notch_Hz, notch_Q );
	}
	return filter_notch_step( &gyro_notch[ num ], &gyro_notch_coeff, input );
}

#endif // BIQUAD_NOTCH_A_HZ


#ifdef BIQUAD_NOTCH_B_HZ

float notch_b_filter( float input, int num )
{
	static FilterNotchCoeff_t gyro_notch_coeff;
	static FilterNotch_t gyro_notch[ 3 ];
	static float notch_Hz, notch_Q;
	if ( notch_Hz != BIQUAD_NOTCH_B_HZ || notch_Q != BIQUAD_NOTCH_B_Q ) {
		notch_Hz = BIQUAD_NOTCH_B_HZ;
		notch_Q = BIQUAD_NOTCH_B_Q;
		filter_notch_coeff( &gyro_notch_coeff, notch_Hz, notch_Q );
	}
	return filter_notch_step( &gyro_notch[ num ], &gyro_notch_coeff, input );
}

#endif // BIQUAD_NOTCH_B_HZ


#ifdef BIQUAD_NOTCH_C_HZ

float notch_c_filter( float input, int num )
{
	static FilterNotchCoeff_t gyro_notch_coeff;
	static FilterNotch_t gyro_notch[ 3 ];
	static float notch_Hz, notch_Q;
	if ( notch_Hz != BIQUAD_NOTCH_C_HZ || notch_Q != BIQUAD_NOTCH_C_Q ) {
		notch_Hz = BIQUAD_NOTCH_C_HZ;
		notch_Q = BIQUAD_NOTCH_C_Q;
		filter_notch_coeff( &gyro_notch_coeff, notch_Hz, notch_Q );
	}
	return filter_notch_step( &gyro_notch[ num ], &gyro_notch_coeff, input );
}

#endif // BIQUAD_NOTCH_C_HZ


#ifdef DYNAMIC_LPF_1ST_HZ

static float one_minus_alpha;
static float gyro_lpf_last[ 3 ];

float gyro_lpf_filter( float in, int num )
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
		if ( filter_Hz != 0.0f ) {
			one_minus_alpha = FILTERCALC( LOOPTIME, 1e6f / filter_Hz );
		}
	}
	lpf( &gyro_lpf_last[ num ], in, one_minus_alpha );
	return gyro_lpf_last[ num ];
}

#endif // DYNAMIC_LPF_1ST_HZ


#ifdef DYNAMIC_LPF_2ND_HZ

static FilterLPF2Coeff_t gyro_lpf2_coeff;
static FilterLPF2_t gyro_lpf2[ 3 ];

float gyro_lpf2_filter( float in, int num )
{
	if ( num == 0 ) { // recalculate coeffs
		const float throttle = rxcopy[ 3 ];
		const float f_base = DYNAMIC_LPF_2ND_HZ_BASE;
		const float f_max = DYNAMIC_LPF_2ND_HZ_MAX;
		const float throttle_breakpoint = DYNAMIC_LPF_2ND_HZ_THROTTLE;
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
		if ( throttle_breakpoint != 0.0f ) {
			float filter_Hz = f_base + throttle / throttle_breakpoint * ( f_max - f_base );
			if ( filter_Hz > f_max ) {
				filter_Hz = f_max;
			} else if ( filter_Hz < f_base ) {
				filter_Hz = f_base;
			}
			filter_lpf2_coeff( &dterm_lpf2_coeff, filter_Hz );
		}
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