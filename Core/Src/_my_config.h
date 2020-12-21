//------------------------------------------------------------------------------
// This file is used to override/customize the options found in config.h
// Edit this file instead of modifying config.h
//------------------------------------------------------------------------------
#ifndef __MY_CONFIG_H__
#define __MY_CONFIG_H__

//------------------------------------------------------------------------------
// Include "reset_defs.h" to disable some SilF4ware features as well as undefine
// numerous definitions that we will redefine here
//------------------------------------------------------------------------------
#include "reset_defs.h" 

//------------------------------------------------------------------------------
// RX protocol and configuration
// Enable only one of the following defines
//------------------------------------------------------------------------------
//#define RX_SILVERLITE_BAYANG_PROTOCOL   // Enable SilverLite SPI Transceiver RX implementation
//#define RX_IBUS // Enable IBUS protocol support on a USART RX pin, double-check rx_ibus.cpp and define one of: FLYSKY_i6_MAPPING, TURNIGY_EVOLUTION_MAPPING
//#define RX_FLYSKY   // Enable FlySky SPI transceiver implementation

#if !defined(RX_SILVERLITE_BAYANG_PROTOCOL) && !defined(RX_IBUS) && !defined(RX_FLYSKY)
    #if defined(MATEKF411RX)
        #define RX_FLYSKY
    #elif defined(CRAZYBEEF3FS)
        #define RX_FLYSKY
    #elif defined(NOX)
        #define RX_SILVERLITE_BAYANG_PROTOCOL
    #endif
#endif

#if !defined(RX_SILVERLITE_BAYANG_PROTOCOL) && !defined(RX_IBUS) && !defined(RX_FLYSKY)
    #warning "No RX implementation was chosen"
#endif

//------------------------------------------------------------------------------
// When using RX_SILVERLITE_BAYANG_PROTOCOL you must specify which transceiver
// module you're using and whether or not you're using 3-wire SPI or 4-wire SPI.
//
// Note:  The software SPI pins used for interfacing with the module are defined 
// in file: trx_spi_config.h
//------------------------------------------------------------------------------
#ifdef RX_SILVERLITE_BAYANG_PROTOCOL

// Define only one of the TRX_??? values below
#define TRX_NRF
//#define TRX_XN297
//#define TRX_XN297L
//#define TRX_LT8900

// Define TRX_SPI_3WIRE if using 3-wire SPI, otherwise comment it out
//#define TRX_SPI_3WIRE

// NRF24L01 module uses 4-wire, not 3-wire SPI
#if defined(TRX_NRF)
    #undef TRX_SPI_3WIRE
#endif

// XN297L module uses 3-wire SPI
#if defined(TRX_XN297L)
    #define TRX_SPI_3WIRE
#endif

#if defined(TRX_XN297L) && !defined(TRX_SPI_3WIRE)
    #error "TRX_XN297L was defined but TRX_SPI_3WIRE wasn't, are you sure about that"
#endif

#if defined(TRX_NRF) && defined(TRX_SPI_3WIRE)
    #error "TRX_NRF was defined but so was TRX_SPI_3WIRE, are you sure about that"
#endif

#endif  // #ifdef RX_SILVERLITE_BAYANG_PROTOCOL


//------------------------------------------------------------------------------
// Rates
//------------------------------------------------------------------------------

// rate in deg/sec for acro mode
#define MAX_RATE            800
#define MAX_RATEYAW         675

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
#define ACRO_EXPO_YAW       0.26

#define ANGLE_EXPO_ROLL     0.55
#define ANGLE_EXPO_PITCH    0.55
#define ANGLE_EXPO_YAW      0.30

//------------------------------------------------------------------------------
// PID term overrides
//------------------------------------------------------------------------------
                        //  Roll    Pitch   Yaw
#define     ACRO_P      {   .040,   .040,   .01     };
#define     ACRO_I      {   .250,   .250,   .50     };
#define     ACRO_D      {   .035,   .035,   .0      };

// 65mm whoop, 0802 19000kv, RPM filtering, 48khz PWM
//#define     ACRO_P      {   .038,   .038,   .01     };
//#define     ACRO_I      {   .140,   .140,   .50     };
//#define     ACRO_D      {   .030,   .030,   .0      };


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
    // Note: The F411 processor on the NOX and MATEKF411RX targets executes the
    // main loop at around 209us when RPM_FILTER is enabled. An OSD update 
    // takes around 71us to 81us which would cause us to exceed the 250 looptime;
    // since this only happens 10 out of every 4000 times I'm more than fine with that. 
    #define RPM_FILTER
    #define LOOPTIME    250     
#elif defined(MATEKF411RX)
    #define RPM_FILTER
    #define LOOPTIME    250     
#elif defined(OMNIBUSF4)
    #define RPM_FILTER
    #define LOOPTIME    250
#elif defined(OMNIBUS)
    #warning "Don't forget to enable RPM_FILTER when you've verified 2K loop will work"
    //#define RPM_FILTER
    #define LOOPTIME    500
#elif defined(CRAZYBEEF3FS)
    #warning "Don't forget to enable RPM_FILTER when you've verified 2K loop will work"
    //#define RPM_FILTER
    #define LOOPTIME    500
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
#define MOTOR_BEEPS_CHANNEL     CH_VID      // SwC/2   ==  MULTI_CHAN_8/DEVO_CHAN_8

#define INVERT_CH_HEADFREE                  // Invert CH_HEADFREE logic
#undef RATES
#define RATES                   CH_HEADFREE // SwD/1   ==  MULTI_CHAN_9/DEVO_CHAN_9

#define TURTLE_MODE             CH_RTH      // SwC/3   ==  MULTI_CHAN_10/DEVO_CHAN_10

#undef LEDS_ON
#define LEDS_ON                 CH_ON

#undef FN_INVERTED
#define FN_INVERTED             CH_OFF      // Default value (DEVO_CHAN_6) conflicts with my channel choice for LEVELMODE

//------------------------------------------------------------------------------
// When you toggle the THROTTLE_KILL_SWITCH to arm the quad, we check to make
// sure the throttle is *BELOW* the value defined by MAX_THROTTLE_TO_ARM. If it
// is above that value then we'll disregard the arm request.
//
// If you'd rather not have this behavior then you can comment out the definition
//
// The throttle value is normalized between 0.0 and 1.0, so you'll want to use
// a very small number
//------------------------------------------------------------------------------
#define MAX_THROTTLE_TO_ARM	0.05f


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
    // You would rotate the Play F4 board 90 degrees clockwise so that the
    // pin 1 dot on the MPU chip would orient correctly and be on the upper
    // left corner
    #define SENSOR_ROTATE_90_CW
#elif defined(OMNIBUSF4)
    #define SENSOR_ROTATE_90_CCW
#elif defined(MATEKF411RX)
    // MPU on HappyModel Crazybee F4 Lite 1S is already correctly oriented
    // so no adjustments necessary
#elif defined(OMNIBUS)
    // MPU on OMNIBUS (F3) is already correctly oriented
    // so no adjustments necessary
#elif defined(CRAZYBEEF3FS)
    // MPU on OMNIBUS (F3) is already correctly oriented
    // so no adjustments necessary
#else
    #error "Unsupported flight controller target. Define mpu orientation here"
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
#elif defined(MATEKF411RX)
    #define MOTOR_BL 3
    #define MOTOR_FL 4
    #define MOTOR_BR 1
    #define MOTOR_FR 2
#elif defined(OMNIBUS)  // TODO
    #warning "Motor order has not been verified"
    #define MOTOR_BL 3
    #define MOTOR_FL 4
    #define MOTOR_BR 1
    #define MOTOR_FR 2
#elif defined(CRAZYBEEF3FS)  // TODO
    #warning "Motor order has not been verified"
    #define MOTOR_BL 3
    #define MOTOR_FL 4
    #define MOTOR_BR 1
    #define MOTOR_FR 2
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
