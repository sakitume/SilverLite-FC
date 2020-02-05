//------------------------------------------------------------------------------
// This file is used to override/customize the options found in config.h
// Edit this file instead of modifying config.h
//------------------------------------------------------------------------------
#ifndef __MY_CONFIG_H__
#define __MY_CONFIG_H__

//------------------------------------------------------------------------------
// Rates
//------------------------------------------------------------------------------
#undef MAX_RATE
#undef MAX_RATEYAW
#undef LEVEL_MAX_ANGLE
#undef LEVEL_MAX_RATE
#undef LOW_RATES_MULTI

// rate in deg/sec for acro mode
#define MAX_RATE 1800
#define MAX_RATEYAW 1800

#define LEVEL_MAX_ANGLE 80
#define LEVEL_MAX_RATE 1800

#define LOW_RATES_MULTI 0.5

//------------------------------------------------------------------------------
// Expo
//  Allowed values are 0.00 to 1.00. A value of 0 means no expo applied
//  The higher the value, the less sensitive near center
//------------------------------------------------------------------------------

#define ACRO_EXPO_ROLL 0.80
#define ACRO_EXPO_PITCH 0.80
#define ACRO_EXPO_YAW 0.60

#define ANGLE_EXPO_ROLL 0.55
#define ANGLE_EXPO_PITCH 0.0
#define ANGLE_EXPO_YAW 0.55

//------------------------------------------------------------------------------
// Battery
//------------------------------------------------------------------------------

#undef WARN_ON_LOW_BATTERY
#define WARN_ON_LOW_BATTERY 3.5

#undef LVC_LOWER_THROTTLE

//------------------------------------------------------------------------------
// Filters
//------------------------------------------------------------------------------
#undef RPM_FILTER   // Will explore this feature later, disable for now

#undef GYRO_LPF_1ST_HZ_BASE
#undef GYRO_LPF_1ST_HZ_MAX
#undef GYRO_LPF_1ST_HZ_THROTTLE

#undef GYRO_LPF_2ND_HZ_BASE
#undef GYRO_LPF_2ND_HZ_MAX
#undef GYRO_LPF_2ND_HZ_THROTTLE

#undef DTERM_LPF_2ND_HZ_BASE
#undef DTERM_LPF_2ND_HZ_MAX
#undef DTERM_LPF_2ND_HZ_THROTTLE

#define GYRO_LPF_1ST_HZ_BASE 90 // Filter frequency at zero throttle.
#define GYRO_LPF_1ST_HZ_MAX 90 // A higher filter frequency than loopfrequency/2.4 causes ripples.
#define GYRO_LPF_1ST_HZ_THROTTLE 0.25 // MAX reached at 1/4 throttle.

#define GYRO_LPF_2ND_HZ_BASE 120 //* ( aux[ FN_INVERTED ] ? 0.75f : 1.0f )
#define GYRO_LPF_2ND_HZ_MAX 120
#define GYRO_LPF_2ND_HZ_THROTTLE 0.25

#define DTERM_LPF_2ND_HZ_BASE 120 //* ( aux[ FN_INVERTED ] ? 0.75f : 1.0f )
#define DTERM_LPF_2ND_HZ_MAX 120
#define DTERM_LPF_2ND_HZ_THROTTLE 0.5

//------------------------------------------------------------------------------
// Switches/Channels
//------------------------------------------------------------------------------

// I'm using a FlySky-i6 with custom firmware and customized nrf24l01 protocols
// as well as a multiprotocol module for other protocols
// 
// These are the i6 switch/channel assignments I typically use for Bayang style
// protocols
//
//  SwA/2   ==  MULTI_CHAN_10/DEVO_CHAN_5   == CH_INV   == ARMING       == THROTTLE_KILL_SWITCH
//  SwB/1   ==  MULTI_CHAN_5/DEVO_CHAN_6    == CH_FLIP  == LEVELMODE    == LEVELMODE
//  SwC/2   ==  MULTI_CHAN_7/DEVO_CHAN_7    == CH_PIC   == RACEMODE     == Not used
//  SwC/3   ==  MULTI_CHAN_8/DEVO_CHAN_8    == CH_VID   == HORIZON      == MOTOR_BEEPS_CHANNEL
//  SwD/2   ==  MULTI_CHAN_9/DEVO_CHAN_9    == CH_HEADFREE  == IDLE_UP  == RATES
// 
//  Note:    If SwC/3 is high, then SwC/2 will also be high

#define INVERT_CH_INV                       // Invert CH_INV logic
#undef THROTTLE_KILL_SWITCH
#define THROTTLE_KILL_SWITCH    CH_INV      // SwA/2   ==  MULTI_CHAN_10/DEVO_CHAN_5

#undef LEVELMODE
#define LEVELMODE               CH_FLIP     // SwB/1   ==  MULTI_CHAN_5/DEVO_CHAN_6

#undef MOTOR_BEEPS_CHANNEL
#define MOTOR_BEEPS_CHANNEL     CH_VID      // SwC/3   ==  MULTI_CHAN_8/DEVO_CHAN_8

#define INVERT_CH_HEADFREE                  // Invert CH_HEADFREE logic
#undef RATES
#define RATES                   CH_HEADFREE // SwD/2   ==  MULTI_CHAN_9/DEVO_CHAN_9

#undef LEDS_ON
#define LEDS_ON                 CH_ON


//------------------------------------------------------------------------------
// RX protocol and configuration
//------------------------------------------------------------------------------
#undef DISPLAY_MAX_USED_LOOP_TIME_INSTEAD_OF_RX_PACKETS
#undef RX_NRF24_BAYANG_TELEMETRY
#undef RX_BAYANG_PROTOCOL_TELEMETRY

#undef RADIO_XN297
#undef RADIO_XN297L

#define RX_SILVERLITE_BAYANG_PROTOCOL

//------------------------------------------------------------------------------
// Gyro orientation
//------------------------------------------------------------------------------
#undef SENSOR_ROTATE_45_CCW
#undef SENSOR_ROTATE_45_CW
#undef SENSOR_ROTATE_90_CW
#undef SENSOR_ROTATE_90_CCW
#undef SENSOR_ROTATE_180
#undef SENSOR_INVERT 

#if defined(NOX)
    #define SENSOR_ROTATE_90_CW
#elif defined(OMNIBUS)
    #define SENSOR_ROTATE_90_CCW
#endif

//------------------------------------------------------------------------------
// Motor order
//------------------------------------------------------------------------------
#if defined(NOX)
    #undef MOTOR_BL
    #undef MOTOR_FL
    #undef MOTOR_BR
    #undef MOTOR_FR

    #define MOTOR_BL 3
    #define MOTOR_FL 4
    #define MOTOR_BR 1
    #define MOTOR_FR 2
#endif

//------------------------------------------------------------------------------
// Blackbox
//------------------------------------------------------------------------------
#undef BLACKBOX_LOGGING // Disable this


#undef LOOPTIME
#define LOOPTIME 250


//------------------------------------------------------------------------------
// SilverLite features
//------------------------------------------------------------------------------
#define     ACRO_P      {   .03,    .03,    .01     };
#define     ACRO_I      {   .25,    .25,    .50     };
#define     ACRO_D      {   .045,   .045,   .0      };

#define     ANGLE_P1    10.
#define     ANGLE_D1    3.0
#define     ANGLE_P2    5.
#define     ANGLE_D2    .0

#endif