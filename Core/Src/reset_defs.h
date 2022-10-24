// This should only be included by _my_config.h. This file disables some SilF4ware features
// as well as undefines numerous definitions that the original SilF4ware code might have defined
// but which _my_config.h will redefine. This is placed here to avoid cluttering _my_config.h

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
// Here we are resetting or clearing various config flags and/or values so that 
// _my_config.h won't be so cluttered

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

// Accelerometer sensor
#undef SENSOR_ROTATE_45_CCW
#undef SENSOR_ROTATE_45_CW
#undef SENSOR_ROTATE_90_CW
#undef SENSOR_ROTATE_90_CCW
#undef SENSOR_ROTATE_180
#undef SENSOR_INVERT 

// Motor order
#undef MOTOR_BL
#undef MOTOR_FL
#undef MOTOR_BR
#undef MOTOR_FR


// Aux channel definitions
#undef FN_INVERTED              // default value is DEVO_CHAN6
#define FN_INVERTED CH_OFF      // Effectively disables FN_INVERTED