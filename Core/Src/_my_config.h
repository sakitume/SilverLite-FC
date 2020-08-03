//------------------------------------------------------------------------------
// This file is used to override/customize the options found in config.h
// Edit this file instead of modifying config.h
//------------------------------------------------------------------------------
#ifndef __MY_CONFIG_H__
#define __MY_CONFIG_H__

//------------------------------------------------------------------------------
// Disable some SilF4ware features
//------------------------------------------------------------------------------
// Disable blackbox, my hardware doesn't have this. Plus this feature only
// builds for OMNIBUSF4 target. Code isn't properly conditionalized to support NOX
#undef BLACKBOX_LOGGING 

// Disable inverted (3D) flight code
#undef  INVERTED_ENABLE

// Disable all SilF4ware RX implementations
#undef DISPLAY_MAX_USED_LOOP_TIME_INSTEAD_OF_RX_PACKETS
#undef RX_NRF24_BAYANG_TELEMETRY
#undef RX_BAYANG_PROTOCOL_TELEMETRY
#undef RADIO_XN297
#undef RADIO_XN297L

//------------------------------------------------------------------------------
// Ignore this section. Here we are resetting or clearing various config flags
// and/or values so that later sections won't be so cluttered; those later
// sections can then focus solely on things you can customize.
//------------------------------------------------------------------------------
// Rates
#undef MAX_RATE
#undef MAX_RATEYAW
#undef LEVEL_MAX_ANGLE
#undef LEVEL_MAX_RATE
#undef LOW_RATES_MULTI

// Battery
#undef WARN_ON_LOW_BATTERY
#undef LVC_LOWER_THROTTLE

// Filters
#undef GYRO_LPF_1ST_HZ_BASE
#undef GYRO_LPF_1ST_HZ_MAX
#undef GYRO_LPF_1ST_HZ_THROTTLE

#undef GYRO_LPF_2ND_HZ_BASE
#undef GYRO_LPF_2ND_HZ_MAX
#undef GYRO_LPF_2ND_HZ_THROTTLE

#undef DTERM_LPF_2ND_HZ_BASE
#undef DTERM_LPF_2ND_HZ_MAX
#undef DTERM_LPF_2ND_HZ_THROTTLE

// Motor order
#undef MOTOR_BL
#undef MOTOR_FL
#undef MOTOR_BR
#undef MOTOR_FR

//------------------------------------------------------------------------------
// RX protocol and configuration
//------------------------------------------------------------------------------
// Enable only one of the following defines
//#define RX_SILVERLITE_BAYANG_PROTOCOL   // Enable SilverLite SPI Transceiver RX implementation
#define RX_IBUS // Enable IBUS protocol support on a USART RX pin

//------------------------------------------------------------------------------
// Rates
//------------------------------------------------------------------------------

// rate in deg/sec for acro mode
#define MAX_RATE            800
#define MAX_RATEYAW         650

#define LEVEL_MAX_ANGLE     80
#define LEVEL_MAX_RATE      900

#define LOW_RATES_MULTI     0.65

//------------------------------------------------------------------------------
// Expo
//  Allowed values are 0.00 to 1.00. A value of 0 means no expo applied
//  The higher the value, the less sensitive near center
//------------------------------------------------------------------------------

#define ACRO_EXPO_ROLL      0.85
#define ACRO_EXPO_PITCH     0.85
#define ACRO_EXPO_YAW       0.60

#define ANGLE_EXPO_ROLL     0.55
#define ANGLE_EXPO_PITCH    0.0
#define ANGLE_EXPO_YAW      0.60

//------------------------------------------------------------------------------
// PID term overrides
//------------------------------------------------------------------------------
                        //  Roll    Pitch   Yaw
#define     ACRO_P      {   .040,   .040,   .01     };
#define     ACRO_I      {   .250,   .250,   .50     };
#define     ACRO_D      {   .035,   .035,   .0      };

// Angle mode P and D terms
#define     ANGLE_P1    10.
#define     ANGLE_D1    3.0

//#define     ANGLE_P1    5.
//#define     ANGLE_D1    .0

//------------------------------------------------------------------------------
// Battery
//------------------------------------------------------------------------------

// Specifies the voltage threshold for when low battery warning occurs
// Comment this out if you wish to disable this feature
#define WARN_ON_LOW_BATTERY 3.5

// Lower throttle when battery below threshold
// Comment this out if you wish to disable this feature
//#define LVC_LOWER_THROTTLE

// Voltage thresholds for use by the "lower throttle on low voltage" feature
// These are ignored if LVC_LOWER_THROTTLE is not defined
#define LVC_LOWER_THROTTLE_VOLTAGE 3.30
#define LVC_LOWER_THROTTLE_VOLTAGE_RAW 2.70
#define LVC_LOWER_THROTTLE_KP 3.0

//------------------------------------------------------------------------------
// LOOPTIME, and filters
//------------------------------------------------------------------------------
// RPM filtering is enabled by default. A 4K loop is also configured (each
// iteration of the loop takes 250us).
//
#undef RPM_FILTER
#undef LOOPTIME
#if defined(NOX)
    // Note: The F411 processor on the NOX target runs typically at 209us when RPM_FILTER
    // is enabled. An OSD update and takes around 71us to 81us which causes us to exceed 
    // the 250 looptime; since this only happens 10 out of every 4000 times I'm
    // more than fine with that. 
    #define RPM_FILTER
    #define LOOPTIME    250     
#elif defined(OMNIBUSF4)
    #define RPM_FILTER
    #define LOOPTIME    250
#else
    #error "Unknown or unsupported flight controller target. Please edit this file"
    #undef RPM_FILTER
    #define LOOPTIME    1000    
#endif

//------------------------------------------------------------------------------
// Filters
//------------------------------------------------------------------------------
#if defined(RPM_FILTER)

#define RPM_FILTER_HZ_MIN           100
#define RPM_FILTER_2ND_HARMONIC     false   // note, that there are 12 notch filters (4 motors * 3 axes) per harmonic
#define RPM_FILTER_3RD_HARMONIC     true
#define RPM_FILTER_Q                6       // -3dB bandwidth = f0 / Q -- but a higher Q also results in a longer settling time

// Dynamic Gyro first order LPF
#define GYRO_LPF_1ST_HZ_BASE        120     // Filter frequency at zero throttle.
#define GYRO_LPF_1ST_HZ_MAX         120     // A higher filter frequency than loopfrequency/2.4 causes ripples.
#define GYRO_LPF_1ST_HZ_THROTTLE    0.25    // MAX reached at 1/4 throttle.

// Dynamic D-Term second order LPF (cannot be turned off)
#define DTERM_LPF_2ND_HZ_BASE       60      //* ( aux[ FN_INVERTED ] ? 0.75f : 1.0f )
#define DTERM_LPF_2ND_HZ_MAX        60
#define DTERM_LPF_2ND_HZ_THROTTLE   0.5

#else

// Dynamic Gyro first order LPF
// Comment out GYRO_LPF_1ST_HZ_BASE if you don't want this filter enabled
#define GYRO_LPF_1ST_HZ_BASE        90      // Filter frequency at zero throttle.
#define GYRO_LPF_1ST_HZ_MAX         90      // A higher filter frequency than loopfrequency/2.4 causes ripples.
#define GYRO_LPF_1ST_HZ_THROTTLE    0.25    // MAX reached at 1/4 throttle.

// Dynamic Gyro second order LPF
// Comment out GYRO_LPF_2ND_HZ_BASE if you don't want filter enabled
#define GYRO_LPF_2ND_HZ_BASE        120     //* ( aux[ FN_INVERTED ] ? 0.75f : 1.0f )
#define GYRO_LPF_2ND_HZ_MAX         120
#define GYRO_LPF_2ND_HZ_THROTTLE    0.25

// Dynamic D-Term second order LPF (cannot be turned off)
#define DTERM_LPF_2ND_HZ_BASE       120     //* ( aux[ FN_INVERTED ] ? 0.75f : 1.0f )
#define DTERM_LPF_2ND_HZ_MAX        120
#define DTERM_LPF_2ND_HZ_THROTTLE   0.5

#endif

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

#define INVERT_CH_INV                       // Invert CH_INV logic (so switch down means ON)
#undef THROTTLE_KILL_SWITCH
#define THROTTLE_KILL_SWITCH    CH_INV      // SwA/2   ==  MULTI_CHAN_10/DEVO_CHAN_5

#undef LEVELMODE
#define LEVELMODE               CH_FLIP     // SwB/1   ==  MULTI_CHAN_5/DEVO_CHAN_6

#undef MOTOR_BEEPS_CHANNEL
#define MOTOR_BEEPS_CHANNEL     CH_VID      // SwC/3   ==  MULTI_CHAN_8/DEVO_CHAN_8

#define INVERT_CH_HEADFREE                  // Invert CH_HEADFREE logic
#undef RATES
#define RATES                   CH_HEADFREE // SwD/1   ==  MULTI_CHAN_9/DEVO_CHAN_9

#undef LEDS_ON
#define LEDS_ON                 CH_ON

#undef FN_INVERTED
#define FN_INVERTED             CH_OFF      // Default value (DEVO_CHAN_6) conflicts with my channel choice for LEVELMODE

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
    // Play F4 board is oriented this way
    #define SENSOR_ROTATE_90_CW
#elif defined(OMNIBUSF4)
    #define SENSOR_ROTATE_90_CCW
#endif

//------------------------------------------------------------------------------
// Motor order
//------------------------------------------------------------------------------
#if defined(NOX)
    #define MOTOR_BL 3
    #define MOTOR_FL 4
    #define MOTOR_BR 1
    #define MOTOR_FR 2
#elif defined(OMNIBUSF4)
    #define MOTOR_BL 2
    #define MOTOR_FL 1
    #define MOTOR_BR 4
    #define MOTOR_FR 3
#else
    #error "Unsupported flight controller target. Define your motor order here"
#endif

//------------------------------------------------------------------------------
// Other important features
//------------------------------------------------------------------------------

// I think having this on lead to the slow/awkward control I first experienced
#undef STICKS_DEADBAND
// But I'll try it again, but this time use 0.01 instead of 0.02
#define STICKS_DEADBAND 0.01f

#endif
