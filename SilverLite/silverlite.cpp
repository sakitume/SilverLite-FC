#include "silverlite.h"
#include "drv_osd.h"
#include "console.h"
#include "config.h"
#include <string.h>

//------------------------------------------------------------------------------
extern "C" {
    extern int packetpersecond;
    extern int hw_crc_errors, b_crc_errors;
    extern int timingfail_errors, skipchannel_errors;
    extern int pkt_hits;
    extern int onground;
    extern char aux[];
    extern float vbattfilt;
    extern bool lowbatt;

    extern float pidkp[];
    extern float pidki[];
    extern float pidkd[];
    extern float apidkp[];
    extern float apidkd[];

#ifdef TURTLE_MODE
    extern bool gTurtleModeActive;
#endif    

#ifdef PID_STICK_TUNING
    extern int current_pid_axis;
    extern int current_pid_term;
    extern bool update_pid_tuning_display;
    extern uint32_t update_flash_saved_display;
#endif

    extern uint32_t gettime();
    extern const char *tprintf(const char* fmt, ...);
}

//------------------------------------------------------------------------------
static void update_osd();

//------------------------------------------------------------------------------
static float _custom_pidkp[] = ACRO_P;
static float _custom_pidki[] = ACRO_I;
static float _custom_pidkd[] = ACRO_D;

//------------------------------------------------------------------------------
void silverlite_init(void)
{
    console_init();
    osd_init();
    update_osd();

    // Update acro PID terms
    for (int i=0; i<3; i++)
    {
        pidkp[i] = _custom_pidkp[i];
        pidki[i] = _custom_pidki[i];
        pidkd[i] = _custom_pidkd[i];
    }

    // Update angle mode PID terms
    apidkp[0] = ANGLE_P1;
    apidkd[0] = ANGLE_D1;
}

//------------------------------------------------------------------------------
static uint32_t osd_ticks = 0;
static uint32_t flyTimeTicks = 0;
static const char *flyModes[] = { "ACRO ", "LEVEL" };
static uint32_t osd_time;
static uint32_t _max_loop_time;
static void update_osd()
{
    int row;
    const char *flyMode;
#ifdef LEVELMODE
    if ( aux[ LEVELMODE ] ) 
    {
        flyMode = flyModes[1];
    }
    else
#endif      
    {
        flyMode = flyModes[0];
    }

    // Fly mode on upper left
    row = 1;
    osd_print(row, 1, flyMode);

    // RSSI (packets per second), on upper right
    const char *s = tprintf("\x01%3d", packetpersecond);
    osd_print(row, 30 - strlen(s) - 1, s);

#if 0 
    // Max loop time and osd time
    s = tprintf("%3d", _max_loop_time);
    osd_print(row+1, 24, s);
    s = tprintf("%3d", osd_time);
    osd_print(row+2, 24, s);
#endif

    // Battery voltage on lower left
    uint8_t maxRows = osd_get_max_rows();
    row = maxRows - 1;
    const int vbatt = vbattfilt * 100 + 0.5f;
    int volts = vbatt/100;
    s = tprintf("%d.%02dV", volts, vbatt%100);
    osd_print(row, 1, s);

    // Flight time on lower right
    uint32_t flyTimeSec =  (flyTimeTicks / (1000000 / LOOPTIME));
    uint32_t minutes = flyTimeSec / 60;
    s = tprintf("\x9C%d:%02d", minutes, flyTimeSec%60);
    osd_print(row, 30 - strlen(s) - 1, s);

    const int kBatteryRow = maxRows/4*3;
    static bool bErasePreviousMsg;

    // Draw or erase "LOW BATTERY" or "CRASH FLIP" as appropriate
#ifdef TURTLE_MODE
    if (lowbatt || gTurtleModeActive)
#else        
    if (lowbatt)
#endif
    {
        bErasePreviousMsg = true;
#ifdef TURTLE_MODE
        const char *msg = gTurtleModeActive ? "CRASH FLIP " : "LOW BATTERY";
#else
        const char *msg = "LOW BATTERY";
#endif
        osd_print(kBatteryRow, (30 - 11) / 2, msg);
    }
    else if (bErasePreviousMsg)
    {
        bErasePreviousMsg = false;
        osd_erase(kBatteryRow, (30 - 11) / 2, 11);
    }

#ifdef PID_STICK_TUNING
    row = 3;
    static uint32_t stick_tuning_active_last;
    if (update_pid_tuning_display)
    {
        stick_tuning_active_last = gettime();
    }
    else if (stick_tuning_active_last)
    {
        // Wait 1/2 second before erasing
        if ((gettime() - stick_tuning_active_last) > (1000 * 500))
        {
            stick_tuning_active_last = 0;
            int x = (30 - 7) / 2;
            osd_erase(row, x, 5);
            osd_erase(row+1, x, 7);
            osd_erase(row+2, x, 10);
        }
    }

    // If stick tuning active within the past 1/2 second
    if (stick_tuning_active_last)
    {
        float term;
        switch (current_pid_term)
        {
            case 0: 
                s = "P";
                term = pidkp[current_pid_axis];
                break;
            case 1: 
                s = "I";    
                term = pidki[current_pid_axis];
                break;
            case 2: 
                s = "D";    
                term = pidkd[current_pid_axis];
                break;
            default: 
                s = "?";   
                break;
        }
        int value = (int)(term * 1000 + 0.5f);

        int x = (30 - 7) / 2;
        const char *temp = tprintf("%01d.%03d", value / 1000, value % 1000);    // length will be 5
        osd_print(row, x, temp);

        temp = tprintf("TERM: %s",s);   // length will be 7
        osd_print(row+1, x, temp);

        switch (current_pid_axis)
        {
#ifdef COMBINE_PITCH_ROLL_PID_TUNING
            case 0:
            case 1: s = "ROLL+PITCH";   break;  // length is 10
#else
            case 0: s = "ROLL";     break;
            case 1: s = "PITCH";    break;
#endif            
            case 2: s = "YAW";      break;
            default: s = "?";       break;
        }
        temp = tprintf("%10s", s);
        osd_print(row+2, x, temp);
    }

    if (update_flash_saved_display)
    {
        row = 6;
        if ((gettime() - update_flash_saved_display) >= (2 * 1000000))
        {
            update_flash_saved_display = 0;
            osd_erase(row, (30-7)/2, 10);
        }
        else
        {
            osd_print(row, (30-7)/2, "FLASHED");
        }
    }
#endif

    osd_refresh();
}

//------------------------------------------------------------------------------
bool silverlite_update()
{
    // Make sure USB VCP is updated or risk losing connection
    console_poll();

    if (!onground)
    {
        flyTimeTicks++;
    }

    // Compute total elapsed milliseconds
    uint32_t ms =  osd_ticks++ / (1000 / LOOPTIME);

    // Every 10th of a second (100ms) we'll update the osd
    if (0 == (ms % 100))
    {
        uint32_t now = gettime();
        update_osd();
        osd_time = gettime() - now;

        // return false to signal to main loop to not count this frame
        // when determining max loop time. OSD update/refresh spikes
        // will always occur and are being measured above with osd_time
        // and therefore shouldn't be considered
        return false;
    }
    return true;
}

//------------------------------------------------------------------------------
// Make sure USB VCP is updated or risk losing connection
void silverlite_poll(void)
{
    console_poll();
}

//------------------------------------------------------------------------------
bool silverlite_postupdate(uint32_t max_used_loop_time)
{
    _max_loop_time = max_used_loop_time;
    static uint32_t secondTimer;
    if ((gettime() - secondTimer) >= 1000000)
    {
        uint32_t now = gettime();

#if 0
        osd_clear(1);
        int row = 1;
        osd_print(row++, 1, tprintf("LOOPTIME: %d", max_used_loop_time));
        osd_print(row++, 1, tprintf("PPS: %3d, HIT: %d", packetpersecond, pkt_hits));
        osd_print(row++, 1, tprintf("HCRC: %2d, BCRC: %d", hw_crc_errors, b_crc_errors));
        osd_print(row++, 1, tprintf("TIMEFAIL: %2d SKIPFAIL: %d", timingfail_errors, skipchannel_errors));
        osd_time = gettime() - now;
        osd_print(row++, 1, tprintf("OSD: %2d", osd_time));
//        osd_print(row++, 1, "\x9B \x9C \x70 \x71");
        // x9C == "fly/mn"
        // x01 == "signal bars"
        osd_print(row++, 1, "\x9B \x9C \x70 \x71 \x01 \x06");
#endif

#if defined(RX_SILVERLITE_BAYANG_PROTOCOL)    
        // Sending this packet takes approx 500us
        console_openPacket();
        console_appendPacket16(max_used_loop_time);
        console_appendPacket16(packetpersecond);
        console_appendPacket16(pkt_hits);
        console_appendPacket16(hw_crc_errors);
        console_appendPacket16(b_crc_errors);
        console_appendPacket16(osd_time);
        console_closePacket(0x01);
#endif

#if defined(RX_IBUS)    
        console_openPacket();
        console_appendPacket16(max_used_loop_time);
        console_appendPacket16(osd_time);
        console_appendPacket16(packetpersecond);
        console_appendPacket16(pkt_hits);
        console_appendPacket16(b_crc_errors);
        console_closePacket(0x02);
#endif

#if defined(RX_FLYSKY)    
        console_openPacket();
        console_appendPacket16(max_used_loop_time);
        console_appendPacket16(osd_time);
        console_appendPacket16(packetpersecond);
        console_appendPacket16(pkt_hits);
        console_appendPacket16(b_crc_errors);
        console_closePacket(0x02);
#endif


        max_used_loop_time = 0;
        secondTimer=  gettime();
        osd_time = secondTimer - now;
        return true;
    }
    
    return false;
}

