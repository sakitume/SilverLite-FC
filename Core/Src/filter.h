float gyro_filter( float in, int num );
float dterm_filter( float in, int num );
void dterm_filter_reset( int holdoff_time_ms );
float throttle_hpf( float in );
void throttle_hpf_reset( int holdoff_time_ms );
